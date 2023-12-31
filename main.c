#include "args.h"
#include "database.h"
#include "term.h"

int main(int argc, char **argv) {
  parseArgs(argc, argv);

  sqlite3 *db = initDatabase();
  initTerm();

  mainLoop(db);

  closeDatabase(db);
  return 0;
}
