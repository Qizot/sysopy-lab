#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char **argv) {
    srand(time(0));
    if (argc < 4) {
        puts("invalid number of arguments, expected: <pipe name> <filename> <N>");
        exit(EXIT_FAILURE);
    }

    char* pipename = argv[1];
    char* filename = argv[2];
    int N = atoi(argv[3]);
    char* buffer = calloc(N + 1, sizeof(char));

    int file = open(filename, O_RDONLY);
    int pipe = open(pipename, O_WRONLY);
    if (pipe == -1) {
        puts("failed to open pipe");
        exit(EXIT_FAILURE);
    }
    if (file == -1) {
        puts("failed to open file");
        exit(EXIT_FAILURE);
    }

    int len = 0;
    while((len = read(file, buffer, N)) > 0) {
        sleep(rand() % 2 + 1);
        char str[N + 20];
        sprintf(str, "#%d#", getpid());
        strncat(str, buffer, len);
        strcat(str, "\n");
        write(pipe, str, strlen(str));
    }


    close(pipe);
    close(file);

    free(buffer);
    return 0;
}