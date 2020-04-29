#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>

#define log(format, ...) {printf("\033[1;31m Main: \033[0m"); printf(format, ##__VA_ARGS__);}
#define FATAL_ERROR(format, ...) { log(format, ##__VA_ARGS__); printf("\n"); exit(1);}
#define TOO_LARGE_DIM 10000
#define min(a, b) a > b ? b : a
#define SIGN 1
#define BLOCK 2
#define INTERLEAVED 3

#define START_TIMER() \
    struct timeval *start = malloc(sizeof(struct timeval)); \
    struct timeval *end = malloc(sizeof(struct timeval)); \
    struct timeval *res = malloc(sizeof(struct timeval)); \
    gettimeofday(start, NULL);

#define STOP_TIMER_AND_RETURN() \
    gettimeofday(end,NULL); \
    timersub(end,start,res); \
    free(start); \
    free(end); \
    return res;

#define EXECUTION_TIME res

#define STOP_TIMER() \
    gettimeofday(end,NULL); \
    timersub(end,start,res); \
    free(start); \
    free(end); \


int** image = NULL;
int height = -1;
int width = -1;
int max_value = -1;
int* results = NULL;
int threads = -1;

void load_image_from_file(char* filename) {
    FILE* file = fopen(filename, "r");

    char buf[64];

    if (fscanf(file, "%s", buf) != 1) {
        FATAL_ERROR("Failed to read file header");
    }
    if (strcmp(buf, "P2") != 0) {
        FATAL_ERROR("Expected P2 as header");
    }

    if (fscanf(file, "%d", &width) != 1) {
        FATAL_ERROR("Failed to read image width from file");
    }
    if (width < 0 || width > TOO_LARGE_DIM) {
        FATAL_ERROR("Invalid image width, got: %d", width);
    }

    if (fscanf(file, "%d", &height) != 1) {
        FATAL_ERROR("Failed to read image height from file");
    }
    if (height < 0 || height > TOO_LARGE_DIM) {
        FATAL_ERROR("Invalid image height, got: %d", height);
    }

    if (fscanf(file, "%d", &max_value) != 1) {
        FATAL_ERROR("Failed to read image height from file");
    }
    if (max_value < 0 || max_value > 255) {
        FATAL_ERROR("Invalid image max color value, got: %d", max_value);
    }

    results = calloc(max_value + 1, sizeof(int));

    image = calloc(height, sizeof(int*));
    for (int i = 0; i < height; i++)
        image[i] = calloc(width, sizeof(int));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (fscanf(file, "%d", &image[i][j]) != 1) {
                FATAL_ERROR("Failed to read pixel color: (%d, %d)", i, j);
            }
            if (image[i][j] < 0 || image[i][j] > max_value) {
                FATAL_ERROR("Read pixel has invalid value: (%d, %d) -> %d", i, j, image[i][j]);
            }
        }
    }
    fclose(file);
}

struct timeval *thread_sign(void *args){
    int *arg = (int*) args;
    int idx = arg[0];
    START_TIMER()

    int step = max_value / threads;
    int from = idx * step;
    int to = min(from + step - 1, max_value);
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            if (image[i][j] >= from && image[i][j] <= to) {
                results[image[i][j]] += 1;
            }
        }
    }

    STOP_TIMER_AND_RETURN()
}

struct timeval *thread_block(void *args){
    int *arg = (int*) args;
    int k = arg[0] + 1;
    START_TIMER()

    int x_from = (k - 1) * ceil( 1.0 * width / threads);
    int x_to = k * ceil(1.0 * width / threads);
    for(int i = 0; i < height; i++){
        for(int j = x_from; j <= x_to; j++){
            results[image[i][j]] += 1;
        }
    }

    STOP_TIMER_AND_RETURN()
}

struct timeval *thread_interleaved(void *args){
    int *arg = (int*) args;
    int k = arg[0] + 1;
    START_TIMER()

    for(int i = 0; i < height; i++){
        for(int j = k - 1; j < width; j += threads){
            results[image[i][j]] += 1;
        }
    }

    STOP_TIMER_AND_RETURN()
}

char* help() {
    return "<program>: <number of threads> <mode: sign | block | inerleaved> <input file> <output file>\n";
}

int parse_mode(char* mode) {
    if (strcmp(mode, "sign") == 0) return SIGN;
    if (strcmp(mode, "block") == 0) return BLOCK;
    if (strcmp(mode, "interleaved") == 0) return INTERLEAVED;
    return -1;
}

void* get_function_by_mode(int mode) {
    switch (mode) {
        case SIGN:
            return thread_sign;
        case BLOCK:
            return thread_block;
        case INTERLEAVED:
            return thread_interleaved;
        default:
            FATAL_ERROR("Invalid mode, can't find requested function");
    }
}

void save_results_to_file(char* filename) {
    FILE* file = fopen(filename, "w");
    for (int i = 0; i <= max_value; i++) {
        fprintf(file, "%d: %d\n", i, results[i]);
    }
    fclose(file);
}


int main(int argc, char** argv) {
    if (argc != 5)
        FATAL_ERROR(help());

    char* nullptr = NULL;
    threads = strtol(argv[1], &nullptr, 10);
    if (*nullptr != 0 || threads <= 0 || threads > 1000) {
        FATAL_ERROR("Invalid number of threads: %d", threads);
    }

    int mode = parse_mode(argv[2]);
    if (mode == -1) {
        FATAL_ERROR("Invalid mode, please see help\n %s", help());
    }

    char* input_filename = argv[3];
    char* output_filename = argv[4];

    load_image_from_file(input_filename);

    START_TIMER()
    pthread_t thread_ids[threads];
    int indexes[threads];

    for (int i = 0; i < threads; i++) {
        indexes[i] = i;
        if (pthread_create(&thread_ids[i], NULL, get_function_by_mode(mode), &indexes[i]) != 0)
            FATAL_ERROR("Failed to create thread");
    }

    for (int i = 0; i < threads; i++) {
        struct timeval* time;
        if (pthread_join(thread_ids[i], (void *) &time) != 0) {
            FATAL_ERROR("Failed to join thread");
        }
        printf("Thread %ld finished in %ld micro seconds\n", thread_ids[i], time->tv_usec);
    }
    STOP_TIMER()
    printf("Whole operation finished in %ld micro seconds\n", EXECUTION_TIME->tv_usec);
    save_results_to_file(output_filename);

    return 0;
}
