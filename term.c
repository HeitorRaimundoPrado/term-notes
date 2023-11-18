#include "utils.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

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
