#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
sqlite3 *initDatabase();
void closeDatabase(sqlite3 *db);
void testSqlite(sqlite3 *db);
int getNumOfNotes(sqlite3 *db);
struct Tag *getTag(sqlite3 *db, int tag);
#endif
