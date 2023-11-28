#include "term.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void printErr_(const char *filename, int line, const char *format, ...) {
  va_list args;
  va_start(args, format);

  fprintf(stderr, "Error on line %d on file %s: \n\r", line, filename);

  vfprintf(stderr, format, args);

  fprintf(stderr, "\n");
  va_end(args);
}

void fatalErr_(const char *filename, int line, int rc, const char *format,
               ...) {

  va_list args;
  va_start(args, format);

  fprintf(stderr, "Fatal error on line %d on file %s: \n", line, filename);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");

  va_end(args);
  exit(rc);
}
