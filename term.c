#include "database.h"
#include "datatypes.h"
#include "utils.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#ifdef _WIN32
#include <windows.h>

#else
#include <termios.h>
#include <unistd.h>

#endif

#ifdef _WIN32
HANDLE hConsole;
CONSOLE_SCREEN_BUFFER_INFO originalConsoleInfo;

void initTerm() {
  printf("\033[?25l");

  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  GetConsoleScreenBufferInfo(hConsole, &originalConsoleInfo);
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

void resetTerm() {
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode;

  SetConsoleTextAttribute(hConsole, originalConsoleInfo.wAttributes);
  SetConsoleCursorPosition(hConsole, originalConsoleInfo.dwCursorPosition);

  GetConsoleMode(hStdin, &mode);

  SetConsoleMode(hStdin, mode | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);

  printf("\033[?25h");
}

#else
static struct termios orig_termios;

void resetTerm() {
  printf("\e[?25h"); // Show cursor
  system("tput rmcup");
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
  printf("\033[?25l");
  system("tput smcup");
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
    printf("%s", lines[i]);
    if (i < num_lines - 1) {
      printf("\n\r");
    }
  }

  fflush(stdout);
}

// returns a formated string for a note with all its information
void generateNoteString(sqlite3 *db, struct Note *note, char **out) {
  struct tm *lastMod_time_info, *created_time_info;
  lastMod_time_info = gmtime(&note->lastmod);
  if (lastMod_time_info == NULL) {
    fatalErr("Error in gmtime");
  }

  created_time_info = gmtime(&note->creation);
  if (created_time_info == NULL) {
    fatalErr("Error in gmtime");
  }

  size_t requiredSize_lastMod =
      strftime(NULL, -1, "%m/%d/%Y - %H:%M", lastMod_time_info);

  char *lastMod = (char *)malloc((requiredSize_lastMod + 1) * sizeof(char));

  strftime(lastMod, requiredSize_lastMod + 1, "%m/%d/%Y - %H:%M",
           lastMod_time_info);

  size_t requiredSize_created =
      strftime(NULL, -1, "%m/%d/%Y - %H:%M", created_time_info);

  char *created = (char *)malloc((requiredSize_created + 1) * sizeof(char));
  strftime(created, requiredSize_created + 1, "%m/%d/%Y - %H:%M",
           created_time_info);

  int size = 0;
  for (int j = 0; j < note->numTags; ++j) {
    struct Tag *tag = getTag(db, note->tags[j]);
    if (j < note->numTags - 1) {
      size += snprintf(NULL, 0, "%s, ", tag->name->str);
    } else {
      size += snprintf(NULL, 0, "%s", tag->name->str);
    }
  }

  char *tags = (char *)calloc((size + 1), sizeof(char));
  for (int j = 0; j < note->numTags; ++j) {
    struct Tag *tag = getTag(db, note->tags[j]);
    if (j < note->numTags - 1) {
      int sizeTag = snprintf(NULL, 0, "%s, ", tag->name->str);
      char *thisTag = (char *)malloc((sizeTag + 1) * sizeof(char));
      sprintf(thisTag, "%s, ", tag->name->str);
      strcat(tags, thisTag);
      free(thisTag);
    } else {
      int sizeTag = snprintf(NULL, 0, "%s", tag->name->str);
      char *thisTag = (char *)malloc((sizeTag + 1) * sizeof(char));
      sprintf(thisTag, "%s", tag->name->str);
      strcat(tags, thisTag);
      free(thisTag);
    }
  }

  int len =
      snprintf(NULL, 0, "%s [last modified: %s] [created: %s] - tags: {%s}",
               note->title->str, lastMod, created, tags);

  *out = NULL;
  *out = (char *)realloc(*out, (len + 1) * sizeof(char));
  sprintf(*out, "%s [last modified: %s] [created: %s] - tags: {%s}",
          note->title->str, lastMod, created, tags);

  free(tags);
  free(created);
  free(lastMod);
}

int *mainView(sqlite3 *db, int _) {

  int numOfNotes = getNumOfNotes(db);
  char **scr = (char **)malloc((numOfNotes + 1) * sizeof(char *));

  struct Note **notes =
      (struct Note **)malloc(numOfNotes * sizeof(struct Note *));

  retrieveAllNotes(db, &notes);

  for (int i = 0; i < numOfNotes; ++i) {
    generateNoteString(db, notes[i], &scr[i]);
  }

  int selectedLine = 0;

  int *ret = (int *)malloc(2 * sizeof(int));
  ret[0] = 0;
  ret[1] = 0;

  int bk = 0;

  const char *msg = "Create new note";
  scr[numOfNotes] = (char *)malloc(strlen(msg) * sizeof(char));
  strcpy(scr[numOfNotes], msg);

  char **scrCpy = (char **)malloc((numOfNotes + 1) * sizeof(char));
  for (int i = 0; i <= numOfNotes; ++i) {
    scrCpy[i] = (char *)malloc((strlen(scr[i]) + 1) * sizeof(char));
    strcpy(scrCpy[i], scr[i]);
  }

  while (1) {
    for (int i = 0; i < 1 + numOfNotes; ++i) {
      if (i == selectedLine) {
        size_t selectLineLen = snprintf(NULL, 0, "\033[7m%s\033[0m", scrCpy[i]);
        scr[i] = (char *)realloc(scr[i], (selectLineLen + 1) * sizeof(char));
        snprintf(scr[i], selectLineLen + 1, "\033[7m%s\033[0m", scrCpy[i]);
      } else {
        strcpy(scr[i], scrCpy[i]);
      }
    }
    draw(numOfNotes + 1, scr);

    char userInp = getInput();
    switch (userInp) {
    case 3:
      ret[0] = 0;
      ret[1] = 0;
      bk = 1;
      break;

    case '\r':
    case '\n':
      bk = 1;

      if (selectedLine < numOfNotes) {
        ret[0] = 2;
        ret[1] = notes[selectedLine]->id;
        break;
      }

      ret[0] = 6;
      break;

    case '\033':
      userInp = getInput();
      switch (userInp) {
      case '[':
        userInp = getInput();
        switch (userInp) {
        case 'A': // up arrow
          selectedLine = MAX(selectedLine - 1, 0);
          break;
        case 'B':
          selectedLine = MIN(selectedLine + 1, numOfNotes);
          break;
        }

        break;
      }

      break;
    }

    if (bk) {
      for (int i = 0; i < numOfNotes + 1; ++i) {
        free(scr[i]);
        free(scrCpy[i]);
      }

      free(scr);
      free(scrCpy);
      break;
    }
  }

  return ret;
}

int *noteView(sqlite3 *db, int note_id) {

  int *ret = (int *)malloc(2 * sizeof(int));

  int cursor_line = 1;

  const int NUM_OF_LINES = 5;
  char **scr = (char **)malloc(NUM_OF_LINES * sizeof(char *));
  char **rendered_scr = (char **)malloc(NUM_OF_LINES * sizeof(char *));

  struct Note *note = (struct Note *)malloc(1 * sizeof(struct Note *));

  retrieveNote(db, note_id, &note);
  generateNoteString(db, note, &scr[0]);

  scr[1] = (char *)malloc(strlen("Edit Note") * sizeof(char));
  strcpy(scr[1], "Edit Note");

  scr[2] = (char *)malloc(strlen("Add Tag") * sizeof(char));
  strcpy(scr[2], "Add Tag");

  scr[3] = (char *)malloc(strlen("Remove Tag") * sizeof(char));
  strcpy(scr[3], "Remove Tag");

  scr[4] = (char *)malloc(strlen("Back <-") * sizeof(char));
  strcpy(scr[4], "Back <-");

  int bk = 0;

  while (1) {
    for (int i = 0; i < NUM_OF_LINES; ++i) {
      if (i == cursor_line) {
        size_t cursor_line_len = snprintf(NULL, 0, "\033[7m%s\033[0m", scr[i]);
        rendered_scr[i] = (char *)malloc((cursor_line_len + 1) * sizeof(char));
        snprintf(rendered_scr[i], cursor_line_len + 1, "\033[7m%s\033[0m",
                 scr[i]);
      }

      else {
        rendered_scr[i] = (char *)malloc(strlen(scr[i]) * sizeof(char));
        strcpy(rendered_scr[i], scr[i]);
      }
    }

    draw(5, rendered_scr);
    char inp = getInput();

    switch (inp) {
    case 3:
      ret[0] = 0;
      ret[1] = 0;
      bk = 1;
      break;

    case '\r':
    case '\n':
      switch (cursor_line) {
      case 1:
        ret[0] = 3;
        ret[1] = note_id;
        break;

      case 2:
        ret[0] = 4;
        ret[1] = note_id;
        break;

      case 3:
        ret[0] = 5;
        ret[1] = note_id;
        break;

      case 4:
        ret[0] = 1;
      }
      bk = 1;
      break;

    case '\033':
      inp = getInput();
      switch (inp) {
      case '[':
        inp = getInput();
        switch (inp) {
        case 'A':
          cursor_line = MAX(cursor_line - 1, 1);
          break;

        case 'B':
          cursor_line = MIN(cursor_line + 1, NUM_OF_LINES - 1);
          break;
        }
        break;
      }
      break;
    }

    if (bk) {
      break;
    }
  }

  for (int i = 0; i < NUM_OF_LINES; ++i) {
    free(scr[i]);
    free(rendered_scr[i]);
  }

  free(rendered_scr);
  free(scr);
  return ret;
}

int *editNoteView(sqlite3 *db, int note) { return 0; }

int *addTagView(sqlite3 *db, int note) { return 0; }

int *removeTagView(sqlite3 *db, int note) { return 0; }

int *addNoteView(sqlite3 *db, int _) { return 0; }

void mainLoop(sqlite3 *db) {
  int *(*currentView)(sqlite3 *, int);
  currentView = &mainView;
  int arg = -1;

  int bk = 0;
  while (1) {
    int *rc = (int *)malloc(2 * sizeof(int));
    rc = currentView(db, arg);

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

    case 4:
      currentView = &addTagView;
      arg = rc[1];

    case 5:
      currentView = &removeTagView;
      arg = rc[1];

    case 6:
      currentView = &addNoteView;
    }

    if (bk) {
      break;
    }
  }
}
