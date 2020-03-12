#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/times.h>
#include <sys/stat.h>

double calculate_time(clock_t start, clock_t end) {
    return (double) (end - start) / sysconf(_SC_CLK_TCK);
}

#define ASSERT_ARGC(n) \
    if (n >= argc) { \
        fprintf(stderr, "%d %d argc exceeded\n", n, argc); \
        exit(EXIT_FAILURE); \
    } \

#define ASSERT_MODE(mode) \
    if (mode != SYS && mode != LIB) { \
       fprintf(stderr, "invalid mode, expected lib or sys\n"); \
       exit(EXIT_FAILURE); \
    } \

#define START_MEASUREMENT(header) \
    char *measurement_header = header; \
    struct tms tms_time[2] = {0}; \
    clock_t real_time[2] = {0}; \
    real_time[0] = times(&tms_time[0]); \

#define END_MEASUREMENT() \
    real_time[1] = times(&tms_time[1]); \
    printf("%s\n", measurement_header); \
    printf("real    user    sys\n"); \
    printf("%lf   ", calculate_time(real_time[0], real_time[1])); \
    printf("%lf   ", calculate_time(tms_time[0].tms_utime, tms_time[1].tms_utime)); \
    printf("%lf ", calculate_time(tms_time[0].tms_stime, tms_time[1].tms_stime)); \
    printf("\n"); \


#define BUFFOR_SIZE 16384

#define LIB 0
#define SYS 1



int check_mode(char* m) {
    if (strcmp(m, "sys") == 0) {
        return SYS;
    }
    if (strcmp(m, "lib") == 0) {
        return LIB;
    }
    return -1;
}

// Generates file with random content
void generate(char* filename, int lines, int chars) {
    if (chars > BUFFOR_SIZE) {
        fprintf(stderr, "chars must be less than buffor size");
        exit(EXIT_FAILURE);
    }
    char *base = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t base_size = strlen(base);

    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "failed to open file");
        exit(EXIT_FAILURE);
    }
    char buf[BUFFOR_SIZE] = {0};

    for (int i = 0; i < lines; i++) {
        for (int j = 0; j < chars; j++) {
            buf[j] = base[rand() % base_size];
        }
        buf[chars] = '\0';
        strcat(buf, "\n");
        fwrite(buf, sizeof(char), chars+ 1, file);
    }
    fclose(file);
}


int load_lib_line(FILE* file, int line, int len, char* buf) {
    long int offset = (len + 1) * sizeof(char);
    fseek(file, line * offset, 0);
    return fread(buf, sizeof(char), len + 1, file);
}

int write_lib_line(FILE* file, int line, int len, char* buf) {
    long int offset = (len + 1) * sizeof(char);
    fseek(file, line * offset, 0);
    return fwrite(buf, sizeof(char), len + 1, file);
}

int load_sys_line(int file, int line, int len, char* buf) {
    long int offset = (len + 1) * sizeof(char);
    lseek(file, line * offset, SEEK_SET);
    return read(file, buf, offset);
}

int write_sys_line(int file, int line, int len, char* buf) {

    long int offset = (len + 1) * sizeof(char);
    lseek(file, line * offset, SEEK_SET);
    return write(file, buf, offset);
}

int partition_lib(FILE* file, char* line, char* pivot, int low, int high, int len) {
    int i = low - 1;
    // load pivot as the last element
    load_lib_line(file, high, len, pivot);
    // perform partition
    for (int j = low; j < high; j++) {
        load_lib_line(file, j, len, line);
        if (line[0] < pivot[0]) {
            i++;
            load_lib_line(file, i, len, pivot);
            write_lib_line(file, j, len, pivot);
            write_lib_line(file, i, len, line);
            // load back pivot
            load_lib_line(file, high, len, pivot);
        }
    }
    load_lib_line(file, i + 1, len, line);
    write_lib_line(file, i + 1, len, pivot);
    write_lib_line(file, high, len, line);
    return i + 1;
}

int partition_sys(int file, char* line, char* pivot, int low, int high, int len) {
    int i = low - 1;
    // load pivot as the last element
    load_sys_line(file, high, len, pivot);
    // perform partition
    for (int j = low; j < high; j++) {
        load_sys_line(file, j, len, line);
        if (line[0] < pivot[0]) {
            i++;
            load_sys_line(file, i, len, pivot);
            write_sys_line(file, j, len, pivot);
            write_sys_line(file, i, len, line);
            // load back pivot
            load_sys_line(file, high, len, pivot);
        }
    }
    load_sys_line(file, i + 1, len, line);
    write_sys_line(file, i + 1, len, pivot);
    write_sys_line(file, high, len, line);
    return i + 1;
}

void lib_quicksort(FILE* file, char* line, char* pivot, int low, int high, int len) {
    if (low < high)
    {
        int pi = partition_lib(file, line, pivot, low, high, len);

        lib_quicksort(file, line, pivot, low, pi - 1, len);
        lib_quicksort(file, line, pivot, pi + 1, high, len);
    }
}

void sys_quicksort(int file, char* line, char* pivot, int low, int high, int len) {
    if (low < high)
    {
        int pi = partition_sys(file, line, pivot, low, high, len);

        sys_quicksort(file, line, pivot, low, pi - 1, len);
        sys_quicksort(file, line, pivot, pi + 1, high, len);
    }
}

int lib_sort(char *path, int lines, int len) {
    FILE *file = fopen(path, "r+");
    char *line = calloc(len + 1, sizeof(char));
    char *pivot = calloc(len + 1, sizeof(char));

    lib_quicksort(file, line, pivot, 0, lines - 1, len);

    free(line);
    free(pivot);
    return 0;
}

int sys_sort(char *path, int lines, int len) {
    int file = open(path, O_RDWR);

    char *line = calloc(len + 1, sizeof(char));
    char *pivot = calloc(len + 1, sizeof(char));

    sys_quicksort(file, line, pivot, 0, lines - 1, len);

    close(file);
    free(line);
    free(pivot);
    return 0;
}

int sort(char *path, int lines, int len, int mode) {
    if (mode == LIB) {
        return lib_sort(path, lines, len);
    }
    if (mode == SYS) {
        return sys_sort(path, lines, len);
    }
    return -1;
}

int sys_copy(char *path, char *dest, int lines, int len) {
    int src = open(path, O_RDONLY);
    int trg = open(dest, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR); // create ifne, wr only, trunc to 0, args with OCR
    char *tmp = malloc(len * sizeof(char));

    for (int i = 0; i < lines; i++){
        if(read(src, tmp,  (len + 1) * sizeof(char)) != (len + 1)) {
            return 1;
        }

        if(write(trg, tmp, (size_t) (len + 1) * sizeof(char)) != (len + 1)) {
            return 1;
        }
    }
    close(src);
    close(trg);
    free(tmp);
    return 0;
}

int lib_copy(char *path, char *dest, int lines, int len) {
    FILE *src = fopen(path, "r");
    FILE *trg = fopen(dest, "w+");
    char *tmp = calloc(len, sizeof(char));

    for (int i = 0; i < lines; i++){
        if(fread(tmp, sizeof(char), (len + 1), src) != (len + 1)) {
            return 1;
        }

        if(fwrite(tmp, sizeof(char), (len + 1), trg) != (len + 1)) {
            return 1;
        }
    }
    fclose(src);
    fclose(trg);
    free(tmp);
    return 0;
}

int copy(char *path, char *dest, int lines, int len, int mode) {
    if (mode == LIB) {
        return lib_copy(path, dest, lines, len);
    }
    if (mode == SYS) {
        return sys_copy(path, dest, lines, len);
    }
    return -1;
}

int main(int argc, char** argv) {
    srand(time(0));
    int i = 1;
    if (strcmp(argv[i], "generate") == 0) {
        ASSERT_ARGC(i + 3)
        int lines = atoi(argv[i+2]);
        int chars = atoi(argv[i+3]);
        generate(argv[i + 1], lines, chars);
    } else if (strcmp(argv[i], "sort") == 0) {
        ASSERT_ARGC(i + 4)
        int mode = check_mode(argv[i + 1]);
        ASSERT_MODE(mode)

        char *file = argv[i + 2];
        int lines = atoi(argv[i + 3]);
        int chars = atoi(argv[i + 4]);
        START_MEASUREMENT("Sorting files")
        sort(file, lines, chars, mode);
        END_MEASUREMENT()

    } else if (strcmp(argv[i], "copy") == 0) {
        ASSERT_ARGC(i + 5)
        int mode = check_mode(argv[i + 1]);
        ASSERT_MODE(mode)

        char *src = argv[i + 2];
        char *dest = argv[i + 3];

        int lines = atoi(argv[i + 4]);
        int chars = atoi(argv[i + 5]);


        START_MEASUREMENT("Copying files")
        copy(src, dest, lines, chars, mode);
        END_MEASUREMENT()
    }

    return 0;
}
