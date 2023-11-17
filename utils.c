#include <stdarg.h>
#include <stdio.h>

void printErr(const char *format, ...) {
  va_list args;
  va_start(args, format);

  fprintf(stderr, "Error: ");

  vfprintf(stderr, format, args);

  va_end(args);

  fprintf(stderr, "\n");
}
