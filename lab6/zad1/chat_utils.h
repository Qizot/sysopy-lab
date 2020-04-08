//
// Created by jakub on 07.04.2020.
//

#ifndef ZAD1_CHAT_UTILS_H
#define ZAD1_CHAT_UTILS_H

#include <sys/types.h>
#include <sys/ipc.h>

#define SERVER_ID 1

key_t get_server_queue_key();
key_t get_client_queue_key();

int parse_number(char* msg);

#endif //ZAD1_CHAT_UTILS_H
