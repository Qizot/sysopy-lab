//
// Created by jakub on 17.03.2020.
//
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

void generate_matrixes(char* first_file, char* second_file, int min, int max) {
    int value_min = -100;
    int value_max = 200;
    int m = rand() % max + min;
    int n = rand() % max + min;
    FILE* first = fopen(first_file, "w");
    FILE* second = fopen(second_file, "w");

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            fprintf(first, "%d ", rand() % (value_max - value_min) + value_min);
        }
        // if (i + 1 < m)
            fprintf(first, "\n");
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            fprintf(second, "%d ", rand() % (value_max - value_min) + value_min);
        }
        // if (i + 1 < n)
            fprintf(second, "\n");
    }
    fclose(first);
    fclose(second);
}

int main(int argc, char** argv) {
    srand(time(NULL));

    char* first_file = argv[1];
    char* second_file = argv[2];
    int min = atoi(argv[3]);
    int max = atoi(argv[4]);
    generate_matrixes(first_file, second_file, min, max);
}

