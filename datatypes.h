#ifndef DATATYPES_H
#define DATATYPES_H
#include <time.h>

struct string {
  int len;
  int allocatedSpace;
  char *str;
};

struct string *initStr();
void s_strcpy(struct string *, const char *);
void ss_strcpy(struct string *, struct string *);
void s_strcat(struct string *, const char *);
void ss_strcat(struct string *, struct string *);
void s_sprintf(struct string *, const char *format, ...);
int s_strlen(struct string *);
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
