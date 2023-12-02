#include "database.h"
#include <stdlib.h>
#include <string.h>

void parseArgs(int argc, char **argv) {
  if (argc == 2 && strcmp(argv[1], "test") == 0) {
    sqlite3 *db = initDatabase();
    testSqlite(db);
    closeDatabase(db);
    exit(0);
  }
}
