//
// Created by jakub on 04.03.2020.
//
#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "library.h"
#include <string.h>

#define BUFFER 1000000

void free_edit_bloc(struct edit_block* block) {
    for (int i = 0; i < block->size; i++) {
        free(block->operations[i]);
    }
    free(block);
}

void delete_manager(struct edit_block_manager* manager) {
    for (int i = 0; i < manager->size; i++) {
        if (manager->edit_blocks[i] != NULL) {
            free_edit_bloc(manager->edit_blocks[i]);
        }
    }
    free(manager->edit_blocks);
    free(manager);
}

struct edit_block_manager* create_block_manager(int size) {
    struct edit_block_manager* manager = calloc(1, sizeof(struct edit_block_manager));
    manager->size = size;
    manager->edit_blocks = calloc(size, sizeof(struct edit_block_manager*));
    return manager;
}

int add_edit_block(struct edit_block_manager* manager, char* tmp_filename) {
    int idx = manager->size;
    for (int i = 0; i < manager->size; i++) {
        if (manager->edit_blocks[i] == NULL) {
            idx = i;
            break;
        }
    }
    if (idx >= manager->size) {
        printf("index out of range");
        return -1;
    }
    if (manager->edit_blocks[idx] != NULL) {
        printf("index is occupied");
        return -1;
    }

    FILE* file = fopen(tmp_filename, "r");
    if (file == NULL) {
        printf("file has not been found");
        return -1;
    }

    struct edit_block* block = calloc(1, sizeof(struct edit_block));

    char** tmp_operations = calloc(BUFFER, sizeof(char*));

    char* line = calloc(BUFFER, sizeof(char));
    char* op_line = calloc(BUFFER, sizeof(char));
    size_t len = 0;



    while (getline(&line, &len, file) != -1) {
        // prasing operation
        if (strncmp(line, "< ", 2) == 0 || strncmp(line, "> ", 2) == 0 || strncmp(line, "---", 3) == 0) {
            strcat(op_line, line);
        } else {

            size_t op_len = strlen(op_line);
            // it's not the first operation block
            if (op_len != 0) {
                int op_idx = block->size - 1;
                tmp_operations[op_idx] = calloc(op_len + 1, sizeof(char));
                strcpy(tmp_operations[op_idx], op_line);
            }
            memset(op_line, 0, op_len);
            strcpy(op_line, line);
            block->size += 1;
        }
    }

    // delete file
    char* cmd = calloc(4 + strlen(tmp_filename), sizeof(char));
    strcpy(cmd, "rm ");
    strcat(cmd, tmp_filename);
    system(cmd);
    free(cmd);

    // create one last operation if op_line is not empty
    if (strlen(op_line) != 0) {
        size_t op_len = strlen(op_line);

        int op_idx = block->size - 1;
        tmp_operations[op_idx] = calloc(op_len + 1, sizeof(char));
        strcpy(tmp_operations[op_idx], op_line);
    }

    // copy operations from tmp array to block
    block->operations = calloc(block->size, sizeof(char*));
    for (int i = 0; i < block->size; i++) {
        block->operations[i] = tmp_operations[i];
    }
    free(tmp_operations);

    manager->edit_blocks[idx] = block;
    return idx;
}

void delete_edit_block(struct edit_block_manager* manager, int idx) {
    if (idx >= manager->size || idx < 0) {
        perror("index out of range");
        exit(-1);
    }
    if (manager->edit_blocks[idx] == NULL) return;

    struct edit_block* block = manager->edit_blocks[idx];
    free_edit_bloc(block);
    manager->edit_blocks[idx] = NULL;
}

void delete_operation(struct edit_block_manager* manager, int block_idx, int operation_idx) {
    if (block_idx >= manager->size || block_idx < 0) {
        perror("block_idx out of range");
        exit(-1);
    }

    struct edit_block* block = manager->edit_blocks[block_idx];
    if (block == NULL) return;

    if (operation_idx >= block->size || operation_idx < 0) {
        perror("operation_idx out of range");
        exit(-1);
    }
    if (block->operations[operation_idx] == NULL) return;
    free(block->operations[operation_idx]);
    block->operations[operation_idx] = NULL;
}

int operations_count(struct edit_block_manager* manager, int block_idx) {
    if (block_idx >= manager->size || block_idx < 0) {
        perror("idx out of range");
        exit(-1);
    }
    struct edit_block* block = manager->edit_blocks[block_idx];
    if (block == NULL) return 0;
    int sum = 0;
    for (int i = 0; i < block->size; i++) {
        sum += block->operations[i] != NULL;
    }
    return sum;
}

char* create_diff_file(char* first_file, char* second_file) {
    char* redirect = " > /tmp/tmp_diff";
    // 8 consists of 'diff' and 3x ' ' + \0
    char* cmd = calloc(8 + strlen(first_file) + strlen(second_file) + strlen(redirect), sizeof(char));
    strcat(cmd, "diff ");
    strcat(cmd, first_file);
    strcat(cmd, " ");
    strcat(cmd, second_file);
    strcat(cmd, redirect);
    system(cmd);
    free(cmd);
    return "/tmp/tmp_diff";
}

