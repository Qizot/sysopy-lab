#include <stdio.h>
#include <stdlib.h>
#include "library.h"
#include <string.h>

int main(int argc, char** argv) {

    struct edit_block_manager* manager = NULL;
    system("pwd");
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
            manager = create_block_manager(blocks);
            printf("Created new table with size: %d\n", manager->size);
        }

        if (strstr(line, "compare_pairs")) {
            strtok(line, " ");
            char* files = strtok(NULL, " ");
            char* firstfile = strtok(files, ":");
            char* secondfile = strtok(NULL, "\n");
            char* tmp_file = create_diff_file(firstfile, secondfile);

            if (manager != NULL) {
                int idx = add_edit_block(manager, tmp_file);
                printf("Created new block with index: %d\n", idx);
            } else {
                printf("Create new table before operating\n");
            }
        }
        if (strstr(line, "remove_block")) {
            strtok(line, " ");
            int idx = atoi(strtok(NULL, " "));
            delete_edit_block(manager, idx);
            printf("Block has been deleted\n");
        }

        if (strstr(line, "remove_operation")) {
            strtok(line, " ");
            int block_idx = atoi(strtok(NULL, " "));
            int op_idx = atoi(strtok(NULL, " "));
            delete_operation(manager, block_idx, op_idx);
            printf("Operation has been deleted\n");
        }
    }



    return 0;
}
