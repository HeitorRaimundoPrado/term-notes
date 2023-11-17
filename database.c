
#include "utils.h"
#include <sqlite3.h>
#include <stdio.h>

sqlite3 *initDatabase() {
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;

  rc = sqlite3_open("tn.db", &db);
  if (rc) {
    printErr("Error opening database");
  }

  return db;
}

void closeDatabase(sqlite3 *db) { sqlite3_close(db); }
