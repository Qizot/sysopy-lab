//
// Created by jakub on 07.04.2020.
//

#ifndef ZAD1_MESSAGE_H
#define ZAD1_MESSAGE_H

#include <sys/types.h>

#define MSG_MAX_SIZE 2048

typedef enum {
    INIT = 1,
    STOP = 2,
    DISCONNECT = 3,
    LIST = 4,
    CONNECT = 5,
    ECHO = 6
} message_type;

struct message {
    long mType;
    pid_t sender;
    char msg[MSG_MAX_SIZE];
};

typedef struct message message_t;

#define MSG_SIZE (sizeof(struct message))

#endif //ZAD1_MESSAGE_H
