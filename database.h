#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
sqlite3 *initDatabase();
void closeDatabase(sqlite3 *db);
#endif
