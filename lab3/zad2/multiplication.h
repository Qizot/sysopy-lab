//
// Created by jakub on 17.03.2020.
//

#ifndef ZAD2_MULTIPLICATION_H
#define ZAD2_MULTIPLICATION_H

#define TMP_FILE 0
#define SHARED_FILE 1

#include <time.h>

struct multiplication_pair {
    char A_matrix[100];
    char B_matrix[100];
    char C_matrix[100];
};

int multiplicate_matrix(int idx, int max_idx, char* first_file, char* second_file, time_t timeout, int mode, char* output_file);
int lines_in_file(FILE* filename);

#endif //ZAD2_MULTIPLICATION_H
