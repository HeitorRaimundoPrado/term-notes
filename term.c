#include "database.h"
#include "datatypes.h"
#include "utils.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>

#else
#include <termios.h>
#include <unistd.h>

#endif

#ifdef _WIN32

void initTerm() {
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode;

  GetConsoleMode(hStdin, &mode);

  SetConsoleMode(hStdin, mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
}

int getInput() {
  char ch;
  DWORD bytesread;
  if (ReadConsoleA(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &bytesread, NULL)) {
    if (bytesread > 0) {
      return ch;
    } else {
      fatalErr("no chars read");
    }
  } else {
    DWORD error = GetLastError();
    LPVOID errorMessage;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, error, 0, (LPSTR)&errorMessage, 0, NULL);

    fatalErr("Error reading from console");
    LocalFree(errorMessage);
  }

  return 0;
}

#else
static struct termios orig_termios;

void resetTerm() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) < 0) {
    fatalErr("error reseting terminal");
  }
}

int getInput() {
  char c_in;
  int bytesread;
  bytesread = read(STDIN_FILENO, &c_in, 1);

  if (bytesread < 0) {
    fatalErr("read error");
  }

  if (bytesread == 0) { // timed out (EOF)
    return 0;
  }

  return c_in;
}

void initTerm() {
  if (tcgetattr(STDIN_FILENO, &orig_termios) < 0) {
    fatalErr("can't get tty settings");
  }

  if (atexit(resetTerm) != 0) {
    fatalErr("can't register tty reset");
  }

  struct termios raw = orig_termios;

  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  // control chars
  raw.c_cc[VMIN] = 5;
  raw.c_cc[VTIME] = 8; /* after 5 bytes or .8 seconds after first byte seen */

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) {
    fatalErr("can't set raw mode");
  }

  write(STDOUT_FILENO, "\033[2J", 4);
}

#endif

void draw(int num_lines, char **lines) {
  printf("\033[2J\033[1;1H");
  for (int i = 0; i < num_lines; ++i) {
    printf("%s\n", lines[i]);
  }
}

int *mainView(sqlite3 *db, int _) {

  int numOfNotes = getNumOfNotes(db);
  char **scr = (char **)malloc((numOfNotes + 1) * sizeof(char *));

  struct Note **notes =
      (struct Note **)malloc(numOfNotes * sizeof(struct Note *));

  for (int i = 0; i < numOfNotes; ++i) {
    struct tm *lastMod_time_info, *created_time_info;
    lastMod_time_info = gmtime(&notes[i]->lastmod);
    created_time_info = gmtime(&notes[i]->creation);
    if (lastMod_time_info == NULL) {
      fprintf(stderr, "1null");
    }
    if (created_time_info == NULL) {
      fprintf(stderr, "2null");
    }

    size_t requiredSize_lastMod =
        strftime(NULL, 0, "%m/%d/%Y - %H:%M", lastMod_time_info);

    char *lastMod = (char *)malloc((requiredSize_lastMod + 1) * sizeof(char));

    strftime(lastMod, requiredSize_lastMod + 1, "%m/%d/%Y - %H:%M",
             lastMod_time_info);

    size_t requiredSize_created =
        strftime(NULL, 0, "%m/%d/%Y - %H:%M", created_time_info);

    char *created = (char *)malloc((requiredSize_created + 1) * sizeof(char));

    strftime(created, requiredSize_created + 1, "%m/%d/%Y - %H: %M",
             created_time_info);

    int size = 0;
    for (int j = 0; j < notes[i]->numTags; ++j) {
      struct Tag *tag = getTag(db, notes[i]->tags[j]);
      if (j < notes[i]->numTags - 1) {
        size += snprintf(NULL, 0, "%s, ", tag->name->str);
      } else {
        size += snprintf(NULL, 0, "%s", tag->name->str);
      }
    }

    char *tags = (char *)malloc((size + 1) * sizeof(char));
    for (int j = 0; j < notes[i]->numTags; ++j) {
      struct Tag *tag = getTag(db, notes[i]->tags[j]);
      if (j < notes[i]->numTags - 1) {
        int sizeTag = snprintf(NULL, 0, "%s, ", tag->name->str);
        char *thisTag = (char *)malloc((sizeTag + 1) * sizeof(char));
        sprintf(thisTag, "%s, ", tag->name->str);
        strcat(tags, thisTag);
      } else {
        int sizeTag = snprintf(NULL, 0, "%s", tag->name->str);
        char *thisTag = (char *)malloc((sizeTag + 1) * sizeof(char));
        sprintf(thisTag, "%s", tag->name->str);
        strcat(tags, thisTag);
      }
    }

    int len =
        snprintf(NULL, 0, "%s [last modified: %s] [created: %s] - tags: {%s}",
                 notes[i]->title->str, lastMod, created, tags);

    scr[i] = (char *)malloc((len + 1) * sizeof(char));
    sprintf(scr[i], "%s [last modified: %s] [created: %s] - tags: {%s}",
            notes[i]->title->str, lastMod, created, tags);
  }

  fprintf(stderr, "gets here");
  fprintf(stderr, "%d", numOfNotes);
  draw(numOfNotes, scr);

  int *ret = (int *)malloc(2 * sizeof(int));
  ret[0] = 0;
  ret[1] = 0;
  return ret;
}

int *noteView(sqlite3 *, int note) { return 0; }

int *editNoteView(sqlite3 *, int note) { return 0; }

void mainLoop(sqlite3 *db) {
  int *(*currentView)(sqlite3 *, int);
  currentView = &mainView;
  int arg = -1;

  int bk = 0;
  while (1) {
    int *rc = (int *)malloc(2 * sizeof(int));
    rc = currentView(db, arg);
    if (bk) {
      fprintf(stderr, "its here");
      break;
    }

    switch (rc[0]) {
    case 0:
      bk = 1;
      break;

    case 1:
      currentView = &mainView;
      break;

    case 2:
      currentView = &noteView;
      arg = rc[1];
      break;

    case 3:
      currentView = &editNoteView;
      arg = rc[1];
      break;
    }
  }
}
