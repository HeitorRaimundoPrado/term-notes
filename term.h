#ifndef TERM_H
#define TERM_H

#include <sqlite3.h>

void mainLoop(sqlite3 *db);
void initTerm();
void resetTerm();

#endif
