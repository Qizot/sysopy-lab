#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

char* create_producer_file(char c, int n) {
    char cmd[80];
    char* file = calloc(20, sizeof(char));
    sprintf(file, "/tmp/%c.txt", c);
    sprintf(cmd, "yes %c | head -n %d | paste -sd'\\0' > %s",c, n, file);
    printf("%s\n", cmd);
    system(cmd);

    return file;
}

int main() {
    char* fifo = "/tmp/tmp_fifo";

    if (mkfifo(fifo, 0666) != 0) {
        perror("failed to create fifo");
        // exit(EXIT_FAILURE);
    }

    pid_t consumer = fork();
    if (consumer == 0) {
        execl("./consumer", "./consumer", fifo, "results.txt", "100", NULL);
        exit(EXIT_SUCCESS);
    }

    int N = 50;

    int chars = 'D' - 'A' + 1;

    pid_t* children = calloc(chars, sizeof(pid_t));
    for (char c = 'A'; c < 'A' + chars; c++) {
      char* file = create_producer_file(c, N * 3);
      pid_t child = fork();
      if (child == 0) {
         execl("./producer", "./producer", fifo, file, "50", NULL);
         exit(EXIT_SUCCESS);
      }
      children[c - 'A'] = child;

      free(file);

    }
    for (int i = 0; i < chars; i++) {
        waitpid(children[i], NULL, 0);
    }

    sleep(2);
    kill(consumer, SIGINT);
    waitpid(consumer, NULL, 0);

    return 0;
}
