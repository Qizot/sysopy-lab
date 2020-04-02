#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>

#define LINE_MAX 256

FILE* pipe_fd;
FILE* file;

void int_handler(int signo) {
    puts("Received SIGINT signal");
    if (pipe_fd) {
        fclose(pipe_fd);
    }
    if (file) {
        fclose(file);
    }
    exit(0);
}

int main(int argc, char** argv) {
    puts("started");
    signal(SIGINT, int_handler);
    if (argc < 4) {
        exit(EXIT_FAILURE);
    }
    char* pipename = argv[1];
    char* filename = argv[2];
    int N = atoi(argv[3]);


    pipe_fd = fopen(pipename, "r+");
    file = fopen(filename, "w");
    if (pipe_fd == NULL) {
        puts("failed to open pipe");
        exit(EXIT_FAILURE);
    }
    if (file == NULL) {
        puts("failed to open file");
        exit(EXIT_FAILURE);
    }

    char* buffer = calloc(N + 1, sizeof(char));
    size_t len = 0;
    setvbuf(file, NULL, _IONBF, 0);
    while((len = fread(buffer, sizeof(char), N, pipe_fd)) > 0) {
        puts("Consumer: reading");
        fwrite(buffer, sizeof(char), len, file);
        fflush(file);
    }
    puts("Cosumer: finished");

    fclose(pipe_fd);
    fclose(file);
    free(buffer);
    return 0;
}