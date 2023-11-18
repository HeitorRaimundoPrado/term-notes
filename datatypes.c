#include "datatypes.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct string *initStr() {
  struct string *ret = (struct string *)malloc(1 * sizeof(struct string));
  ret->str = (char *)malloc(200 * sizeof(char));
  ret->allocatedSpace = 200;
  ret->len = 0;
  return ret;
}

void strcat(struct string *str1, const char *str2) {
  if (str1->len + strlen(str2) >= str1->allocatedSpace) {
    str1->str = (char *)realloc(str1->str, (str1->len + strlen(str2) + 200) *
                                               sizeof(char));
    if (str1->str == NULL) {
      fatalErr("memory allocation error");
    }
  }

  str1->len += strlen(str2);
  str1->allocatedSpace += strlen(str2) + 200;
  strcat(str1->str, str2);
}

void strcat(struct string *str1, struct string *str2) {
  if (str1->len + str2->len >= str1->allocatedSpace) {
    str1->str = (char *)realloc(str1->str,
                                (str1->len + str2->len + 200) * sizeof(char));

    if (str1->str == NULL) {
      fatalErr("memory allocation error");
    }
  }

  str1->len += str2->len;
  str1->allocatedSpace += str2->len + 200;
}

void strcpy(struct string *str1, const char *str2) {
  if (strlen(str2) >= str1->allocatedSpace) {
    free(str1->str);
    str1->str = (char *)malloc((strlen(str2) + 200) * sizeof(char));
    if (str1->str == NULL) {
      fatalErr("memory allocation error");
    }
  }

  str1->len = strlen(str2);
  str1->allocatedSpace = strlen(str2) + 200;
}

void strcpy(struct string *str1, struct string *str2) {
  if (str2->len >= str1->allocatedSpace) {
    free(str1->str);
    str1->str = (char *)malloc(str2->allocatedSpace);
    if (str1->str == NULL) {
      fatalErr("memory allocation error");
    }
  }

  str1->len = str2->len;
  str1->allocatedSpace = str2->allocatedSpace;
}

void sprintf(struct string *str, const char *format, ...) {
  va_list args;
  va_start(args, format);

  int sz = vsnprintf(NULL, 0, format, args);
  if (sz >= str->allocatedSpace) {
    str->allocatedSpace += sz + str->allocatedSpace;
    str->str = (char *)realloc(str->str, str->allocatedSpace * sizeof(char));
    if (str->str == NULL) {
      fatalErr("memory allocation error");
    }
  }

  int charsSubbed = vsnprintf(str->str, str->allocatedSpace, format, args);
  if (charsSubbed < 0) {
    fatalErr("error in sprintf");
  }

  str->len = charsSubbed;

  va_end(args);
}

int strlen(struct string *str) { return str->len; }

void cleanup(struct string *str) {
  free(str->str);
  free(str);
}
