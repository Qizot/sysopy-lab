#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "library.h"
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <zconf.h>

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


int main(int argc, char** argv) {

    struct tms tms_time[2];
    clock_t real_time[2];

    struct edit_block_manager* manager = NULL;
    char* line;
    size_t len = 0;
    while(getline(&line, &len, stdin) != -1) {
        if (strstr(line, "create_table")) {
            strtok(line, " ");
            int blocks = atoi(strtok(NULL, " "));
            if (manager != NULL) {
                delete_manager(manager);
                manager = NULL;
            }

            real_time[0] = times(&tms_time[0]);
            manager = create_block_manager(blocks);
            real_time[1] = times(&tms_time[1]);

            print_time("create_table", real_time, tms_time);

        }

        if (strstr(line, "compare_pairs")) {
            strtok(line, " ");
            char* file;
            int len = 0;
            char** files = calloc(1000, sizeof(char*));

            while((file = strtok(NULL, " "))) {
                files[len] = file;
                len += 1;
            }

            real_time[0] = times(&tms_time[0]);
            for (int i = 0; i < len; i++) {
                char* firstfile = strtok(files[i], ":");
                char* secondfile = strtok(NULL, "\n");
                char* tmp_file = create_diff_file(firstfile, secondfile);
                add_edit_block(manager, tmp_file);
            }

            free(files);

            real_time[1] = times(&tms_time[1]);
            print_time("add_edit_block", real_time, tms_time);
        }

        if (strstr(line, "remove_block")) {
            strtok(line, " ");
            int idx = atoi(strtok(NULL, " "));

            real_time[0] = times(&tms_time[0]);
            delete_edit_block(manager, idx);
            real_time[1] = times(&tms_time[1]);

            print_time("delete_edit_block", real_time, tms_time);
        }

        if (strstr(line, "remove_operation")) {
            strtok(line, " ");
            int block_idx = atoi(strtok(NULL, " "));
            int op_idx = atoi(strtok(NULL, " "));

            real_time[0] = times(&tms_time[0]);
            delete_operation(manager, block_idx, op_idx);
            real_time[1] = times(&tms_time[1]);

            print_time("delete_operation", real_time, tms_time);
        }
    }



    return 0;
}
