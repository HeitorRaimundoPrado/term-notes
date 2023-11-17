#include "term.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void printErr_(const char *filname, int line, const char *format, ...) {
  va_list args;
  va_start(args, format);

  fprintf(stderr, "Error: ");

  vfprintf(stderr, format, args);

  fprintf(stderr, "\n");
  va_end(args);
}

void fatalErrVaArgs(int rc, const char *format, va_list args) {
  fprintf(stderr, "Fatal error: ");

  vfprintf(stderr, format, args);

  fprintf(stderr, "\n");
}

void fatalErr(int rc, const char *format, ...) {
  va_list args;
  va_start(args, format);

  fatalErrVaArgs(rc, format, args);

  va_end(args);
  exit(rc);
}
