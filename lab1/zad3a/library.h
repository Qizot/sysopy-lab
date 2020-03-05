//
// Created by jakub on 04.03.2020.
//

#ifndef LAB1_LIBRARY_H
#define LAB1_LIBRARY_H

struct edit_block_manager {
    int size;
    struct edit_block** edit_blocks;
};

struct edit_block {
    int size;
    char** operations;
};

void delete_manager(struct edit_block_manager* manager);

struct edit_block_manager* create_block_manager(int size);

int add_edit_block(struct edit_block_manager* manager, char* tmp_filename);

void delete_edit_block(struct edit_block_manager* manager, int idx);

void delete_operation(struct edit_block_manager* manager, int block_idx, int operation_idx);

int operations_count(struct edit_block_manager* manager, int block_idx);

char* create_diff_file(char* first_file, char* second_file);






#endif //LAB1_LIBRARY_H
