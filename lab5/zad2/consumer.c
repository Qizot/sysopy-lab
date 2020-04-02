#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>

#define LINE_MAX 256


void int_handler(int signo) {
    puts("Received SIGINT signal");
    exit(0);
}

int main(int argc, char** argv) {
    signal(SIGINT, int_handler);
    if (argc < 4) {
        puts("invalid number of arguments");
        exit(EXIT_FAILURE);
    }
    char* pipename = argv[1];
    char* filename = argv[2];
    int N = atoi(argv[3]);


    int pipe_fd = open(pipename, O_RDONLY);
    if (pipe_fd == -1) {
        perror("failed to open pipe");
        exit(EXIT_FAILURE);
    }

    int file = open(filename, O_CREAT | O_WRONLY, 0644);
    if (file == -1) {
        perror("failed to open file");
        exit(EXIT_FAILURE);
    }

    char* buffer = calloc(N + 1, sizeof(char));
    size_t len = 0;
    while((len = read(pipe_fd, buffer, N)) > 0) {
        write(file, buffer, len);
    }

    close(pipe_fd);
    close(file);
    free(buffer);
    return 0;
}