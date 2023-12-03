#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "datatypes.h"
#include "utils.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define STATIC_ASSERT(COND, MSG, MSG_FAIL)                                     \
  fprintf(stderr, "ASSERTION ON LINE %d ON FILE %s:\n", __LINE__, __FILE__);   \
  if (COND) {                                                                  \
    fprintf(stderr, "%s\n\n", MSG);                                            \
  } else {                                                                     \
    fprintf(stderr, "%s\n\n", MSG_FAIL);                                       \
    exit(1);                                                                   \
  }

void closeDatabase(sqlite3 *db) {
  if (db != NULL) {
    int rc = sqlite3_close(db);
    if (rc != SQLITE_OK) {
      fatalErr(sqlite3_errmsg(db));
    }
  }
}

sqlite3 *initDatabase() {
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;

  rc = sqlite3_open("tn.db", &db);
  if (rc) {
    printErr("Error opening database %s", sqlite3_errmsg(db));
  }

  char sql[] = /*"CREATE TABLE IF NOT EXISTS tags("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "name TEXT UNIQUE"
               ");"

               "CREATE TABLE IF NOT EXISTS note ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "creation DATE,"
               "lastmod DATE,"
               "title TEXT,"
               "content TEXT"
               ");"

               "CREATE TABLE IF NOT EXISTS note_tags("
               "note_id INTEGER,"
               "tag_id INTEGER,"
               "PRIMARY KEY (note_id, tag_id),"
               "FOREIGN KEY (note_id) REFERENCES note(id),"
               "FOREIGN KEY (tag_id) REFERENCES tags(id)"
               ");";*/
      "CREATE TABLE IF NOT EXISTS tags("
      "id INTEGER PRIMARY KEY,"
      "name TEXT UNIQUE"
      ");"

      "CREATE TABLE IF NOT EXISTS note ("
      "id INTEGER PRIMARY KEY,"
      "creation INTEGER,"
      "lastmod INTEGER,"
      "title TEXT,"
      "content TEXT"
      ");"

      "CREATE TABLE IF NOT EXISTS note_tags("
      "note_id INTEGER,"
      "tag_id INTEGER,"
      "PRIMARY KEY (note_id, tag_id),"
      "FOREIGN KEY (note_id) REFERENCES note(id),"
      "FOREIGN KEY (tag_id) REFERENCES tags(id)"
      ");";

  rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    printErr(zErrMsg);
    fatalErr("error creating database");
  }

  return db;
}

void insertNote(struct Note *note, sqlite3 *db) {
  struct string *sql = initStr();

  s_sprintf(sql,
            "INSERT INTO note(creation, lastmod, title, content) "
            "VALUES(%lld, "
            "%lld, '%s', '%s')",
            note->creation, note->lastmod, note->title->str,
            note->content->str);

  char *zErrMsg;
  int rc = sqlite3_exec(db, sql->str, NULL, 0, &zErrMsg);
  note->id = sqlite3_last_insert_rowid(db);

  if (rc != SQLITE_OK) {
    printErr(zErrMsg);
    fatalErr("error inserting values into db");
  }

  for (int i = 0; i < note->numTags; ++i) {
    s_sprintf(sql, "INSERT INTO note_tags VALUES(%lld, %lld)", note->id,
              note->tags[i]);

    rc = sqlite3_exec(db, sql->str, NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
      printErr(zErrMsg);
      fatalErr("error inserting values into db");
    }
  }
}

int insertTag(struct Tag *tag, sqlite3 *db) {
  struct string *sql = initStr();

  s_sprintf(sql, "INSERT INTO tags(name) VALUES('%s')", tag->name->str);

  char *zErrMsg;
  int rc = sqlite3_exec(db, sql->str, NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    printErr(zErrMsg);
    fatalErr("error inserting values into db");
  }

  return sqlite3_last_insert_rowid(db);
}

void addTagToNote(struct Tag *tag, struct Note *note, sqlite3 *db) {
  struct string *sql = initStr();
  insertTag(tag, db);
  s_sprintf(sql, "INSERT INTO note_tags(%lld, %lld)", note->id, tag->id);
  char *zErrMsg;
  int rc = sqlite3_exec(db, sql->str, NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    printErr(zErrMsg);
    fatalErr("error inserting values into db");
  }
}

int getNumOfNotes(sqlite3 *db) {
  int rowCount = 0;
  struct string *sql = initStr();
  s_sprintf(sql, "SELECT COUNT(*) FROM note");
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql->str, -1, &stmt, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      rowCount = sqlite3_column_int(stmt, 0);
    } else {
      fatalErr("%s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
  } else {
    fatalErr("Error preparing query: %s", sqlite3_errmsg(db));
  }

  return rowCount;
}

void retrieveAllNotes(sqlite3 *db, struct Note ***data) {
  sqlite3_stmt *stmt;
  const char *pragma_query = "PRAGMA table_info(note)";
  int rc = sqlite3_prepare_v2(db, pragma_query, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    printErr(sqlite3_errmsg(db));
    fatalErr("error getting table info");
  }

  sqlite3_finalize(stmt);

  const char *pragma_tags = "PRAGMA table_info(note_tags)";
  rc = sqlite3_prepare_v2(db, pragma_query, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    printErr(sqlite3_errmsg(db));
    fatalErr("error getting table info");
  }

  sqlite3_finalize(stmt);

  const char *query = "SELECT * FROM note";
  rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    printErr(sqlite3_errmsg(db));
    fatalErr("error preparing statement");
  }

  int row_count = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    row_count++;
  }
  *data = (struct Note **)malloc(sizeof(struct Note *) * row_count);

  sqlite3_reset(stmt);

  int row = 0;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int column_count = sqlite3_column_count(stmt);
    (*data)[row] = (struct Note *)malloc(1 * sizeof(struct Note));
    for (int i = 0; i < column_count; ++i) {
      const char *column_name = sqlite3_column_name(stmt, i);
      if (strcmp(column_name, "id") == 0) {
        (*data)[row]->id = sqlite3_column_int(stmt, i);

        struct string *note_tag_query = initStr();
        sqlite3_stmt *stmt2;

        s_sprintf(note_tag_query, "SELECT * FROM note_tags WHERE note_id=%d",
                  (*data)[row]->id);

        rc = sqlite3_prepare_v2(db, note_tag_query->str, -1, &stmt2, NULL);
        if (rc != SQLITE_OK) {
          printErr(sqlite3_errmsg(db));
          fatalErr("error preparing statement");
        }

        int row_count_nt = 0;
        while ((rc = sqlite3_step(stmt2) == SQLITE_ROW))
          row_count_nt++;

        (*data)[row]->numTags = row_count_nt;
        (*data)[row]->tags = (int *)malloc(row_count_nt * sizeof(int));
        int k = 0;
        while ((rc = sqlite3_step(stmt2)) == SQLITE_ROW) {
          int column_count_nt = sqlite3_column_count(stmt2);

          for (int j = 0; j < column_count_nt; ++j) {
            const char *column_name_tags = sqlite3_column_name(stmt2, j);
            if (strcmp(column_name_tags, "tag_id") == 0) {
              (*data)[row]->tags[k++] = sqlite3_column_int(stmt2, j);
            }
          }
        }

        sqlite3_finalize(stmt2);
      } else if (strcmp(column_name, "title") == 0) {
        (*data)[row]->title = initStr();
        s_strcpy((*data)[row]->title,
                 (const char *)sqlite3_column_text(stmt, i));

      } else if (strcmp(column_name, "content") == 0) {
        (*data)[row]->content = initStr();
        s_strcpy((*data)[row]->content,
                 (const char *)sqlite3_column_text(stmt, i));

      } else if (strcmp(column_name, "lastmod") == 0) {
        (*data)[row]->lastmod = sqlite3_column_int(stmt, i);

      } else if (strcmp(column_name, "creation") == 0) {
        (*data)[row]->creation = sqlite3_column_int(stmt, i);
      }
    }
    row++;
  }

  sqlite3_finalize(stmt);
}

int isDatabaseEmpty(sqlite3 *db) {
  sqlite3_stmt *stmt;
  const char *query = "SELECT COUNT(*) FROM sqlite_master";
  if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
    fatalErr("Error preparing statement: %s", sqlite3_errmsg(db));
  }

  int result = sqlite3_step(stmt);

  if (result == SQLITE_ROW) {
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if (count == 0) {
      return 0;
    } else {
      return 1;
    }
  } else {
    sqlite3_finalize(stmt);
    fatalErr("Error executing statement: %s", sqlite3_errmsg(db));
    return -1; // fatalErr will already exit(1), this is only so that the
               // compiler doesn't throw warnings
  }
}

struct Tag *getTag(sqlite3 *db, int note) {
  sqlite3_stmt *stmt;
  struct string *query = initStr();
  s_sprintf(query, "SELECT * FROM note_tags WHERE note_id=%d;", note);

  if (sqlite3_prepare_v2(db, query->str, -1, &stmt, NULL) != SQLITE_OK) {
    fatalErr("Error preparing statement: %s", sqlite3_errmsg(db));
  }

  int rc;
  struct Tag *ret = (struct Tag *)malloc(1 * sizeof(struct Tag));
  if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    int column_count = sqlite3_column_count(stmt);
    for (int i = 0; i < column_count; ++i) {
      const char *column_name = sqlite3_column_name(stmt, i);
      if (strcmp(column_name, "tag_id") == 0) {
        struct string *tagQuery = initStr();
        s_sprintf(tagQuery, "SELECT * FROM tags WHERE id=%d;",
                  sqlite3_column_int(stmt, i));

        sqlite3_stmt *stmt2;
        if (sqlite3_prepare_v2(db, tagQuery->str, -1, &stmt2, NULL) !=
            SQLITE_OK) {
          fatalErr("Error preparing statement: %s", sqlite3_errmsg(db));
        }

        int rcTag;
        while ((rcTag = sqlite3_step(stmt2)) == SQLITE_ROW) {
          int column_count_tag = sqlite3_column_count(stmt2);
          for (int j = 0; j < column_count_tag; ++j) {

            const char *column_name_tag = sqlite3_column_name(stmt2, j);
            if (strcmp(column_name_tag, "name") == 0) {
              ret->name = initStr();
              s_strcpy(ret->name, (const char *)sqlite3_column_text(stmt2, j));
            } else if (strcmp(column_name_tag, "id") == 0) {
              ret->id = sqlite3_column_int(stmt2, j);
            }
          }
        }
        sqlite3_finalize(stmt2);
      }
    }
  } else if (rc != SQLITE_DONE) {
    fatalErr("%s\n", sqlite3_errmsg(db));
  }
  sqlite3_finalize(stmt);
  return ret;
}

void testSqlite(sqlite3 *db) {
  if (!isDatabaseEmpty(db)) {
    fatalErr("The tests are supposed to be executed in an empty database\nBe "
             "sure to erase the database before continuing with the test\n");
  }

  fprintf(stderr, "Initializing testing sequence\n\n");
  struct Note *note1, *note2;
  struct Tag *tag1;

  struct string *str = initStr();
  s_sprintf(str, "%d, %s, %.2f", 3, "hello", 3.12);
  STATIC_ASSERT(strcmp(str->str, "3, hello, 3.12") == 0,
                "Test s_sprintf passed", "test s_sprintf failed");

  tag1 = (struct Tag *)malloc(1 * sizeof(struct Tag));
  note1 = (struct Note *)malloc(1 * sizeof(struct Note));
  note2 = (struct Note *)malloc(1 * sizeof(struct Note));

  tag1->name = initStr();

  s_strcpy(tag1->name, "hello world");
  STATIC_ASSERT(strcmp(tag1->name->str, "hello world") == 0, "test1 passed",
                "test1 failed");

  tag1->id = -1;

  note1->title = initStr();
  s_strcpy(note1->title, "title1");
  STATIC_ASSERT(strcmp(note1->title->str, "title1") == 0, "test2 passed",
                "test2 failed");

  note1->id = -1;

  note1->numTags = 1;

  note1->tags = (int *)malloc(1 * sizeof(int));
  note1->tags = &(tag1->id);

  note1->content = initStr();
  s_strcpy(note1->content, "this is the first note");

  time_t current_time = time(NULL);

  note1->lastmod = current_time;
  note1->creation = current_time;

  note2->title = initStr();
  s_strcpy(note2->title, "title2");

  note2->id = -1;

  note2->numTags = 0;
  note2->tags = NULL;

  note2->content = initStr();
  s_strcpy(note2->content, "this is the second note");

  note2->lastmod = current_time;
  note2->creation = current_time;

  tag1->id = insertTag(tag1, db);

  insertNote(note1, db);
  insertNote(note2, db);

  struct Tag *testTag = getTag(db, 1);

  STATIC_ASSERT(note1->lastmod == current_time, "Test lastmod passed",
                "Test lastmod failed");
  STATIC_ASSERT(testTag->id == tag1->id, "Test getTag id passed",
                "Test getTag id failed");

  STATIC_ASSERT(strcmp(testTag->name->str, "hello world") == 0,
                "Test getTag name passed", "Test getTag name failed");

  int numOfNotes = getNumOfNotes(db);

  STATIC_ASSERT(numOfNotes == 2, "Test3 Passed", "test3 failed");

  struct Note **notes;

  retrieveAllNotes(db, &notes);
  for (int i = 0; i < numOfNotes; ++i) {

    if (notes[i]->id == 1) {
      STATIC_ASSERT(strcmp(notes[i]->title->str, "title1") == 0, "Test4 passed",
                    "Test4 failed");
      STATIC_ASSERT(strcmp(notes[i]->content->str, "this is the first note") ==
                        0,
                    "Test5 passed", "Test5 failed");
    }

    else if (notes[i]->id == 2) {
      STATIC_ASSERT(strcmp(notes[i]->title->str, "title2") == 0, "Test6 passed",
                    "Test6 failed");

      STATIC_ASSERT(strcmp(notes[i]->content->str, "this is the second note") ==
                        0,
                    "Test7 passed", "Test7 failed");
    }
  }

  fprintf(stderr, "erasing all values from database\n\n");
  fprintf(stderr, "finishing testing sequence\n");
}
