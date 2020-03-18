//
// Created by jakub on 17.03.2020.
//
#define _GNU_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/file.h>
#include <limits.h>
#include <zconf.h>
#include "multiplication.h"

#define BUFFER 1 << 15

int lines_in_file(FILE* file) {
    int pos = ftell(file);
    int lines = 0;

    fseek(file, 0, SEEK_SET);
    for (char c = getc(file); c != EOF; c = getc(file))
        if (c == '\n')
            lines += 1;
    fseek(file,pos, SEEK_SET);
    return lines;
}

int numbers_in_line(FILE* file) {
    int pos = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buf = NULL;
    size_t size;
    getline(&buf, &size, file);

    int numbers = 0;

    char* n = strtok(buf, " ");
    do {
        numbers++;
    } while ((n = strtok(NULL, " ")));
    free(buf);

    fseek(file, pos, SEEK_SET);
    // -1 because number in file end with " \n" and it parses one last space
    return numbers - 1;
}

void write_tmp_file(char* filename, int** C, int n, int m) {
    FILE* file = fopen(filename, "w");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            int val = C[i][j];
            fprintf(file, "%d ",val);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

void write_shared_file(char* filename, int** C, int n, int from, int to, int file_n, int file_m) {
    FILE* file = fopen(filename, "r+");
    fseek(file, 0, SEEK_SET);
    int result = flock(fileno(file), LOCK_EX);

    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    int** F = calloc(file_n, sizeof(int*));
    for (int i = 0; i < file_n; i++) {
        F[i] = calloc(file_m, sizeof(int));
    }

    for (int i = 0; i < file_n; i++) {
        for (int j = 0; j < file_m; j++) {
            F[i][j] = INT_MAX;
        }
    }
    if (file_size > 0) {
        char* line = NULL;
        size_t len;
        len = 0;
        for (int j = 0; j < file_n; j++) {
            if (getline(&line, &len, file) > 0) {
                if (strlen(line) < 100) {
                    printf("encountered short string: %s", line);
                }
                if (strcmp(line, "") == 0) break;
                int i = 0;
                char* token = strtok(line, " ");
                do {
                    if (strcmp(token, "\n") == 0) {
                        break;
                    } else if (strcmp(token, "x") == 0) {
                        F[j][i] = INT_MAX;
                    } else {
                        int num = atoi(token);
                        F[j][i] = num;
                    }
                    i++;
                } while ((token = strtok(NULL, " ")));
                free(line);
                line = NULL;
            }
        }
    }

    for (int i = 0; i < n; i++) {
       for (int j = from; j < to; j++) {
           F[i][j] = C[i][j - from];
       }
    }
    fseek(file, 0, SEEK_SET);

    for (int i = 0; i < file_n; i++) {
        for (int j = 0; j < file_m; j++) {
            if (F[i][j] == INT_MAX) {
                fprintf(file, "x ");
            } else {
                fprintf(file, "%d ", F[i][j]);
            }
        }
        fprintf(file, "\n");
    }
    for (int i = 0; i < file_m; i++) {
        free(F[i]);
    }
    free(F);
    fflush(file);
    flock(fileno(file), LOCK_UN);
    fclose(file);
}

int multiplicate_matrix(int idx, int max_idx, char* first_file, char* second_file, time_t timeout, int mode, char* output_file) {
    FILE* A_file = fopen(first_file, "r");
    FILE* B_file = fopen(second_file, "r");
    int a_n = lines_in_file(A_file);
    int a_m = numbers_in_line(A_file);
    int b_n = lines_in_file(B_file);
    int b_m = numbers_in_line(B_file);

    int step = b_m / max_idx;
    int from = step * idx;
    int to = from + step;
    // we are the last process so take everything up to the last column of B matrix
    if (idx == max_idx - 1)
        to = b_m;

    int** A = calloc(a_n, sizeof(int*));
    for (int i = 0; i < a_n; i++) {
       A[i] = calloc(a_m, sizeof(int));
    }

    int** B = calloc(b_n, sizeof(int*));
    for (int i = 0; i < b_n; i++) {
        B[i] = calloc(b_m, sizeof(int));
    }

    int** C = calloc(a_n, sizeof(int*));
    for (int i = 0; i < a_n; i++) {
        C[i] = calloc((to - from),  sizeof(int));
    }
    // int C[n][to - from + 1];
    int multiplications = 0;

    for (int i = 0; i < a_n; i++) {
        for (int j = 0; j < a_m; j++) {
            fscanf(A_file, "%d", &A[i][j]);
        }
    }

    for (int i = 0; i < b_n; i++) {
        for (int j = 0; j < b_m; j++) {
            fscanf(B_file, "%d", &B[i][j]);
        }
    }

    time_t current_time = time(NULL);

    for (int i = 0; i < a_n; i++) {
        for (int k = from; k < to; k++) {
            int sum = 0;
            for (int j = 0; j < b_n; j++) {
                int a = A[i][j];
                int b = B[j][k];
                sum += a * b;
            }
            multiplications += b_n;
            C[i][k - from] = sum;
        }
        current_time = time(NULL);
        if (current_time > timeout) {
            printf("Process %d has timed out, multiplications: %d but expected: %d\n", getpid(), multiplications, a_n * b_n * (to - from));
            goto write_to_file;
        }
    }

    write_to_file:
    if (mode == TMP_FILE) {
        write_tmp_file(output_file, C, a_n, to - from);
    } else if (mode == SHARED_FILE) {
        write_shared_file(output_file, C, a_n, from, to, a_n, b_m);
    }

    for (int i = 0; i < a_n; i++)
        free(A[i]);
    free(A);
    for (int i = 0; i < b_n; i++)
        free(B[i]);
    free(B);
    for (int i = 0; i < a_n; i++)
        free(C[i]);
    free(C);

    fclose(A_file);
    fclose(B_file);
    return multiplications;
}