#define _GNU_SOURCE 500
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include "multiplication.h"

pid_t create_fork(int idx, int max_idx, char* first_file, char* second_file, int timeout, int mode, char* output_file) {
    pid_t child_pid = fork();
    if (child_pid == 0) {
        char out[80];
        if (mode == TMP_FILE) {
            sprintf(out, "./tmp_files/%d.txt", idx);
        } else {
            sprintf(out, "%s", output_file);
        }
        exit(multiplicate_matrix(idx, max_idx, first_file, second_file, timeout, mode, out));
    } else if (child_pid > 0) {
        return child_pid;
    }
    return -1;
}

int parse_mode(char* mode) {
    if (strcmp(mode, "tmp_file") == 0) {
        return TMP_FILE;
    } else if (strcmp(mode, "shared_file") == 0) {
        return SHARED_FILE;
    }
    return -1;
}

void concat_tmp_files(int processes, char* output_file) {

    pid_t paste_child = fork();
    int fd = open(output_file, O_RDWR | O_CREAT, 0777);
    if (paste_child == 0) {
        dup2(fd, 1);

        char* args[processes + 3];
        args[0] = "paste";
        args[1] = "-d'\0'";
        args[processes + 2] = 0;
        for (int i = 0; i < processes; i++) {
            char* filename = calloc(100, sizeof(char));
            sprintf(filename, "./tmp_files/%d.txt", i);
            args[2 + i] = filename;
        }
        execvp("paste", args);

        for (int i = 0; i < processes; i++) {
            free(args[2 + i]);
        }
    }
    system("rm -rf ./tmp_files/");
    close(fd);
}

int main(int argc, char** argv) {
    char *const operations_file = argv[1];
    char A_matrix[100];
    char B_matrix[100];
    char C_matrix[100];
    FILE* file = fopen(operations_file, "r");
    fscanf(file, "%s %s %s", A_matrix, B_matrix, C_matrix);
    fclose(file);

    int processes = atoi(argv[2]);
    int timeout = atoi(argv[3]);
    int mode = parse_mode(argv[4]);
    if (mode == TMP_FILE) {
        system("mkdir tmp_files");
    }


    pid_t *children = calloc(processes, sizeof(pid_t));

    for (int i = 0; i < processes; i++) {
        children[i] = create_fork(i, processes, A_matrix, B_matrix, timeout, mode, C_matrix);
    }

    for (int i = 0; i < processes; i++) {
        int status;
        waitpid(children[i], &status, 0);
        printf("Proces %d wykonał %d mnożeń macierzy\n", children[i], WEXITSTATUS(status));
    }
    if (mode == TMP_FILE) {
        concat_tmp_files(processes, C_matrix);
    }
    free(children);
    return 0;
}
