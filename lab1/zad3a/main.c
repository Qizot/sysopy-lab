#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "library.h"
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <zconf.h>

#define CHECK_ARGC(N) \
    if (N >= argc) { \
        perror("Ivalid number of arguments"); \
        exit(0); \
    }\

double elapsed_time(clock_t start, clock_t end) {
    return (double) (end - start) / sysconf(_SC_CLK_TCK);
}



void print_time(char* header, clock_t real[2], struct tms* tms_time) {
    printf("%s\n", header);
    printf("real time    user time    sys time\n");
    printf("%lf   ", elapsed_time(real[0], real[1]));
    printf("%lf   ", elapsed_time(tms_time[0].tms_utime, tms_time[1].tms_utime));

    printf("%lf ", elapsed_time(tms_time[0].tms_stime, tms_time[1].tms_stime));
    printf("\n\n");
}

const char* const commands[] = {"create_table", "compare_pairs", "remove_operation", "remove_block"};

int is_command(char* string) {
    for (int i = 0; i < 4; i++) {
        if (strcmp(commands[i], string) == 0)
            return 1;
    }
    return 0;
}

int main(int argc, char** argv) {

    struct tms tms_time[2];
    clock_t real_time[2];

    struct edit_block_manager* manager = NULL;
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "create_table") == 0) {
            i++;
            CHECK_ARGC(i);

            int blocks = atoi(argv[i]);
            
            real_time[0] = times(&tms_time[0]);
            manager = create_block_manager(blocks);
            real_time[1] = times(&tms_time[1]);

            print_time("create_table", real_time, tms_time);
            i++;
        }
        else if (strcmp(argv[i], "compare_pairs") == 0) {
            i++;
            CHECK_ARGC(i);

            real_time[0] = times(&tms_time[0]);

            while (i < argc && !is_command(argv[i])) {
                char* firstfile = strtok(argv[i], ":");
                char* secondfile = strtok(NULL, "\n");
                char* tmp_file = create_diff_file(firstfile, secondfile);
                add_edit_block(manager, tmp_file);
                i++;
            }

            real_time[1] = times(&tms_time[1]);
            
            print_time("add_edit_block", real_time, tms_time);
        }
        else if (strcmp(argv[i], "remove_block") == 0) {
            i++;
            CHECK_ARGC(i);  
            int idx = atoi(argv[i]);
            real_time[0] = times(&tms_time[0]);
            delete_edit_block(manager, idx);
            real_time[1] = times(&tms_time[1]);

            print_time("delete_edit_block", real_time, tms_time);

            i++;
        }
        else if (strcmp(argv[i], "remove_operation") == 0) {
            i += 2;
            CHECK_ARGC(i);
            
            int block_idx = atoi(argv[i-1]);
            int op_idx = atoi(argv[i]);
            
            real_time[0] = times(&tms_time[0]);
            delete_operation(manager, block_idx, op_idx);
            real_time[1] = times(&tms_time[1]);

            
            print_time("delete_operation", real_time, tms_time);
            i++;
        } else {
            i++;
        }
    }


    return 0;
}
