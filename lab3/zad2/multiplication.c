//
// Created by jakub on 17.03.2020.
//
#define _GNU_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "multiplication.h"

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

int multiplicate_matrix(int idx, int max_idx, char* first_file, char* second_file, int timeout, int mode, char* output_file) {
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

    int A[a_n][a_m];
    int B[b_n][b_m];
    int** C = malloc(a_n * sizeof(int*));
    for (int i = 0; i < a_n; i++) {
        C[i] = malloc((to - from) * sizeof(int));
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

    for (int i = 0; i < a_n; i++) {
        for (int k = from; k < to; k++) {
            int sum = 0;
            for (int j = 0; j < b_n; j++) {
                int a = A[i][j];
                int b = B[j][k];
                sum += a * b;
            }
            multiplications += 1;
            C[i][k - from] = sum;
        }
    }

    if (mode == TMP_FILE) {
        write_tmp_file(output_file, C, a_n, to - from);
    } else if (mode == SHARED_FILE) {
        // TODO
    }
    for (int i = 0; i < a_n; i++)
        free(C[i]);
    free(C);
    fclose(A_file);
    fclose(B_file);
    return multiplications;
}