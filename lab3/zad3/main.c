#define _GNU_SOURCE 500
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include "multiplication.h"

int apply_limits(int cpu_limit, int mem_limit) {
    struct rlimit cpu_rlimit;
    struct rlimit mem_rlimit;

    if (getrlimit(RLIMIT_CPU, &cpu_rlimit) == -1) {
        printf("Error while getting limit: %s", strerror(errno));
    }

    if (getrlimit(RLIMIT_AS, &mem_rlimit) == -1) {
        printf("Error while getting limit: %s", strerror(errno));
    }

    cpu_rlimit.rlim_cur = cpu_rlimit.rlim_max = cpu_limit;
    mem_rlimit.rlim_cur = mem_rlimit.rlim_max = (1 << 20) * mem_limit;

    if (setrlimit(RLIMIT_CPU, &cpu_rlimit) == -1) {
        printf("Unable to set cpu limit: %s\n", strerror(errno));
        exit(-1);
    }
    if (setrlimit(RLIMIT_AS, &mem_rlimit) == -1) {
        printf("Unable to set mem limit: %s\n", strerror(errno));
        exit(-1);
    }
    return 0;
}

pid_t create_fork(int idx, int max_idx, struct multiplication_pair* pairs, int pairs_size, time_t timeout, int mode, int cpu_limit, int mem_limit) {
    pid_t child_pid = fork();
    if (child_pid == 0) {
        apply_limits(cpu_limit, mem_limit);
        int multiplications = 0;
        for (int i = 0; i < pairs_size; i++) {
            char out[200];
            if (mode == TMP_FILE) {
                sprintf(out, "./tmp_files/%d/%d.txt", i, idx);
            } else {
                sprintf(out, "%s", pairs[i].C_matrix);
            }
            int val =  multiplicate_matrix(idx, max_idx, pairs[i].A_matrix, pairs[i].B_matrix, timeout, mode, out);
            multiplications += val;
            if (val == 0) {
                exit(multiplications);
            }
        }
        exit(multiplications);
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

void concat_tmp_files(int processes, int pair_idx, char* output_file) {

    pid_t paste_child = fork();
    int fd = open(output_file, O_RDWR | O_CREAT, 0777);
    if (paste_child == 0) {
        dup2(fd, 1);

        char* args[processes + 3];
        args[0] = "paste";
        args[1] = "-d\\0";
        args[processes + 2] = 0;
        for (int i = 0; i < processes; i++) {
            char* filename = calloc(100, sizeof(char));
            sprintf(filename, "./tmp_files/%d/%d.txt", pair_idx, i);
            args[2 + i] = filename;
        }
        execvp("paste", args);

        for (int i = 0; i < processes; i++) {
            free(args[2 + i]);
        }
    }
    close(fd);
}

struct multiplication_pair* parse_file(char* filename, int* size) {
   FILE* file = fopen(filename, "r");
   int lines = lines_in_file(file);
   *size = lines;
   struct multiplication_pair* pairs = calloc(lines, sizeof(struct multiplication_pair));
   int i = 0;
   while (fscanf(file, "%s %s %s\n", pairs[i].A_matrix, pairs[i].B_matrix, pairs[i].C_matrix) == 3) i++;
   fclose(file);
   return pairs;
}

int main(int argc, char** argv) {
    char *const operations_file = argv[1];
    int pairs_size = 0;
    struct multiplication_pair* pairs = parse_file(operations_file, &pairs_size);

    int processes = atoi(argv[2]);
    int timeout = atoi(argv[3]);
    int mode = parse_mode(argv[4]);
    int cpu_limit = atoi(argv[5]);
    int mem_limit = atoi(argv[6]);

    if (mode == TMP_FILE) {
        system("mkdir tmp_files");
        for (int i = 0; i < pairs_size; i++) {
            char cmd[200];
            sprintf(cmd, "mkdir tmp_files/%d", i);
            system(cmd);
        }
    } else if (mode == SHARED_FILE) {
        for (int i = 0; i < pairs_size; i++) {
            char cmd[200] = "> ";
            strcat(cmd, pairs[i].C_matrix);
            system(cmd);
        }
    }


    pid_t *children = calloc(processes, sizeof(pid_t));
    time_t current_time = time(NULL);
    for (int i = 0; i < processes; i++) {
        children[i] = create_fork(i, processes, pairs, pairs_size, current_time + timeout, mode, cpu_limit, mem_limit);
    }

    struct rusage prev_usage;
    getrusage(RUSAGE_CHILDREN, &prev_usage);
    struct rusage current_usage;
    struct timeval ru_utime;
    struct timeval ru_stime;
    for (int i = 0; i < processes; i++) {
        int status;
        waitpid(children[i], &status, 0);

        getrusage(RUSAGE_CHILDREN, &current_usage);
        timersub(&current_usage.ru_stime,&prev_usage.ru_stime,&ru_stime);
        timersub(&current_usage.ru_utime,&prev_usage.ru_utime,&ru_utime);

        printf("Process %d has done %d matrix multiplications\nSystem time: %d.%06d User time: %d.%06d \n\n", 
            children[i], 
            WEXITSTATUS(status),
            (int)ru_stime.tv_sec,(int)ru_stime.tv_usec,
            (int)ru_utime.tv_sec,(int)ru_utime.tv_usec
        );

        prev_usage = current_usage;


    }
    if (mode == TMP_FILE) {
        for (int i = 0; i < pairs_size; i++) {
            concat_tmp_files(processes, i, pairs[i].C_matrix);
        }
        system("rm -rf tmp_files");
    }

    free(children);

    return 0;
}
