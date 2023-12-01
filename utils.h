#ifndef UTILS_H
#define UTILS_H

#define printErr(format, ...)                                                  \
  if (sizeof((char[]){#__VA_ARGS__}) == 1) {                                   \
    printErr_(__FILE__, __LINE__, "%s", format);                               \
  } else {                                                                     \
    printErr_(__FILE__, __LINE__, format, ##__VA_ARGS__);                      \
  }

#define fatalErr(format, ...)                                                  \
  if (sizeof((char[]){#__VA_ARGS__}) == 1) {                                   \
    fatalErr_(__FILE__, __LINE__, "%s", format);                               \
  } else {                                                                     \
    fatalErr_(__FILE__, __LINE__, format, ##__VA_ARGS__);                      \
  }

void printErr_(const char *filename, int line, const char *format, ...);
void fatalErr_(const char *filename, int line, const char *format, ...);

#endif
