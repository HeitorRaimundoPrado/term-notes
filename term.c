#include "utils.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  SetConsoleMode(hStdin, mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT) |
                             ENABLE_VIRTUAL_TERMINAL_PROCESSING)
}

int getInput() {
  char ch;
  DWORD bytesread;
  if (ReadConsoleA(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &bytesRead, NULL)) {
    if (bytesRead > 0) {
      return ch;
    } else {
      fatalErr("no chars read")
    }
  } else {
    DWORD error = getLastError();
    LPVOID errorMessage;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, error, 0, (LPSTR)&errorMessage, 0, NULL);

    fatalErr("Error reading from console");
    localFree(errorMessage);
  }

  return 0;
}

void draw(int num_lines, char **lines) {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  WriteConsoleA(hConsole, "\033[2J", 4);
  WriteConsoleA(hConsole, "\033[1;1H", 6);

  for (int i = 0; i < num_lines; ++i) {
    DWORD charsWritten;
    WriteConsoleA(hConsole, lines[i], lstrlenA(lines[i]), &charsWritten, NULL);
    WriteConsoleA(hConsole, "\n\r", 1, &charsWritten, NULL);
  }
}

#else
static struct termios orig_termios;

void resetTerm() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) < 0)
    fatalErr("error reseting terminal");
}

void draw(int num_lines, char **lines) {
  write(STDOUT_FILENO, "\033[2J", 4);
  write(STDOUT_FILENO, "\033[1;1H", 6);
  for (int i = 0; i < num_lines; ++i) {
    write(STDOUT_FILENO, "\n\r", 2);
    write(STDOUT_FILENO, lines[i], strlen(lines[i]));
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

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0)
    fatalErr("can't set raw mode");

  write(STDOUT_FILENO, "\033[2J", 4);
}

#endif

void mainLoop(sqlite3 *db) {
  char **msg = (char **)malloc(2 * sizeof(char *));
  msg[0] = (char *)malloc(5 * sizeof(char));
  msg[1] = (char *)malloc(5 * sizeof(char));

  strcpy(msg[1], "world");
  while (1) {
    int c = getInput();

    char *n = (char *)malloc(30 * sizeof(char));

    sprintf(n, "%d", c);

    strcpy(msg[0], n);
    if (c == 0x03) // ctrl + c
      break;

    draw(1, msg);
  }
}
