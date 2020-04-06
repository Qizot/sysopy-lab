#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
  if (argc < 2) {
    puts("invalid number of arguments, expected: <filename>");
    exit(EXIT_FAILURE);
  }

  char cmd[100];
  strcpy(cmd, "cat ");
  strcat(cmd, argv[1]);
  strcat(cmd, " | sort");

  FILE* file = popen(cmd, "r");
  if (file == NULL) {
    perror("failed to popen()");
    exit(EXIT_FAILURE);
  }

  char buffer[1024];
  while (fgets(buffer, sizeof(buffer), file)) {
    printf("%s", buffer);
  }
}

