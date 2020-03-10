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
        fprintf(stderr, "argc exceeded\n"); \
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
        return LIB;
    }
    if (strcmp(m, "lib") == 0) {
        return SYS;
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


int lib_sort(char *path, int line, int len) {
    FILE *file = fopen(path, "r+");
    char *reg1 = calloc(len + 1, sizeof(char));
    char *reg2 = calloc(len + 1, sizeof(char));

    long int offset = (long int) ((len + 1) * sizeof(char));
    for (int i = 0; i < line; i++) {
        fseek(file, i * offset, SEEK_SET); // 0 offset from beg
        if (fread(reg1, sizeof(char), line + 1, file) != (len + 1)) { // ill formed file, should have lines * (len + 1) characters
            return 1;
        }

        for (int j = 0; j < i; j++) {
            fseek(file, j * offset, 0);
            if (fread(reg2, sizeof(char), (len + 1), file) != (len + 1)) {
                return 1;
            }
            if (reg2[0] > reg1[0]) {
                fseek(file, j * offset, 0);
                if (fwrite(reg1, sizeof(char), (len + 1), file) != (len + 1)) {
                    return 1;
                }
                fseek(file, i * offset, 0);
                if (fwrite(reg2, sizeof(char), (len + 1), file) != (len + 1)) {
                    return 1;
                }
                char *tmp = reg1;
                reg1 = reg2;
                reg2 = tmp;
            }
        }
    }

    fclose(file);
    free(reg1);
    free(reg2);
    return 0;
}

int sys_sort(char *path, int line, int len) {
    int file = open(path, O_RDWR);

    char *reg1 = calloc(len + 1, sizeof(char));
    char *reg2 = calloc(len + 1, sizeof(char));

    long int offset = (long int) ((len + 1) * sizeof(char));
    int bytes = sizeof(char) * (len + 1);

    for (int i = 0; i < line; i++) {
        lseek(file, i * offset, SEEK_SET); // SEEK_SET - from beg

        if (read(file, reg1, bytes) != (len + 1)) {
            return 1;
        }

        for (int j = 0; j < i; j++) {
            lseek(file, j * offset, SEEK_SET);
            if (read(file, reg2, bytes) != (len + 1)) {
                return 1;
            }
            if (reg2[0] > reg1[0]) {
                lseek(file, j * offset, 0);
                if (write(file, reg1, bytes) != (len + 1)) {
                    return 1;
                }
                lseek(file, i * offset, 0);
                if (write(file, reg2, bytes) != (len + 1)) {
                    return 1;
                }
                char *tmp = reg1;
                reg1 = reg2;
                reg2 = tmp;
            }
        }
    }

    close(file);
    free(reg1);
    free(reg2);
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
