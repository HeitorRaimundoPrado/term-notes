#include "database.h"
#include "term.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc == 2 && strcmp(argv[1], "test") == 0) {
    sqlite3 *db = initDatabase();
    testSqlite(db);
    closeDatabase(db);
    return 0;
  }

  sqlite3 *db = initDatabase();
  initTerm();

  mainLoop(db);

  closeDatabase(db);
  return 0;
}
