//
// Created by jakub on 17.03.2020.
//

#ifndef ZAD2_MULTIPLICATION_H
#define ZAD2_MULTIPLICATION_H

#define TMP_FILE 0
#define SHARED_FILE 1

#include <time.h>

int multiplicate_matrix(int idx, int max_idx, char* first_file, char* second_file, time_t timeout, int mode, char* output_file);

#endif //ZAD2_MULTIPLICATION_H
