#ifndef DATATYPES_H
#define DATATYPES_H
#include <time.h>

struct string {
  int len;
  int allocatedSpace;
  char *str;
};

struct string *initStr();
void strcpy(struct string *, const char *);
void strcpy(struct string *, struct string *);
void strcat(struct string *, const char *);
void strcat(struct string *, struct string *);
void sprintf(struct string *, const char *format, ...);
void cleanup(struct string *);

struct Tag {
  int id;
  struct string name;
};

struct Note {
  int id;
  struct string *title;
  time_t creation;
  time_t lastmod;
  int numTags;
  struct Tag **tags;
  struct string *content;
};
#endif
