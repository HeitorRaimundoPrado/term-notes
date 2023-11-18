
#include "datatypes.h"
#include "utils.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  char sql[] = "CREATE TABLE IF NOT EXISTS tags("
               "id INTEGER PRIMARY KEY,"
               "name TEXT UNIQUE"
               ");"

               "CREATE TABLE IF NOT EXISTS note ("
               "id INTEGER PRIMARY KEY,"
               "creation DATE,"
               "lastmod DATE,"
               "title TEXT,"
               "content TEXT"
               ");"

               "CREATE TABLE note_tags("
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

  sprintf(sql, "INSERT INTO note VALUES(%lld, %lld, '%s', '%s')",
          note->creation, note->lastmod, note->title, note->content);

  char *zErrMsg;
  int rc = sqlite3_exec(db, sql->str, NULL, 0, &zErrMsg);

  if (rc != SQLITE_OK) {
    printErr(zErrMsg);
    fatalErr("error inserting values into db");
  }

  for (int i = 0; i < note->numTags; ++i) {
    sprintf(sql, "INSERT INTO note_tags(%lld, %lld)", note->id,
            note->tags[i]->id);

    rc = sqlite3_exec(db, sql->str, NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
      printErr(zErrMsg);
      fatalErr("error inserting values into db");
    }
  }
}

void insertTag(struct Tag *tag, sqlite3 *db) {
  struct string *sql = initStr();
  sprintf(sql, "INSERT INTO tags VALUES('%s')", tag->name);
}
