//
// Created by jakub on 07.04.2020.
//
#include "chat_utils.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

key_t get_server_queue_key() {
    char *home = getenv("HOME");
    if (!home) {
        puts("[ERROR] couldn't get HOME env");
        return -1;
    }
    key_t s_key = ftok(home, 1);
    if (s_key == -1) {
        puts("[ERROR] couldn't get HOME env");
    }
    return s_key;

}

key_t get_client_queue_key() {
    char *home = getenv("HOME");
    if (!home) {
        puts("[ERROR] couldn't get HOME env");
        return -1;
    }
    key_t s_key = ftok(home, getpid());
    if (s_key == -1) {
        puts("[ERROR] couldn't get HOME env");
    }
    return s_key;
}

int parse_number(char *str) {
    if (!str) {
        return -1;
    }
    char *tmp;
    int result = (int) strtol(str, &tmp, 10);
    if (strcmp(tmp, str) != 0) {
        return result;
    } else {
        return -1;
    }
}