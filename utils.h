#ifndef UTILS_H
#define UTILS_H

#define printErr(format, ...)                                                  \
  printErr_(__FILE__, __LINE__, format, ##__VA_ARGS__)

void printErr_(const char *filename, int line, const char *format, ...);
void fatalErr(const char *format, ...);

#endif
