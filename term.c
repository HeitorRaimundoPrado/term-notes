#include <stdio.h>

void mainLoop() {
  while (1) {
    char name[11];
    printf("Enter your name: ");
    scanf("%10s", name);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
    }
    printf("Hello %s\n", name);
  }
}
