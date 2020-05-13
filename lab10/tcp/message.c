//
// Created by jakub on 12.05.2020.
//
#include "message.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int get_message_size(message_type_t type) {
    switch (type) {
        case CONNECTED:
        case DISCONNECTED:
        case WAITING:
        case PING:
        case PONG:
        case INFO:
            return sizeof(info_message_t);
        case CONNECT:
        case DISCONNECT:
            return sizeof(action_message_t);
        case MOVE:
            return sizeof(move_message_t);
        case START_GAME:
            return sizeof(start_game_message_t);
    }
    return -1;
}

void send_message(int fd, basic_message_t *msg, int size) {
    if (write(fd, &msg->type, sizeof(msg->type)) == -1)
        FATAL_ERROR("Failed to send msg type");
    if (write(fd, msg, size) != size)
        FATAL_ERROR("Failed to send msg, sent: %d bytes", size);
}

void *read_message(int fd, int *type) {
    int msgtype;
    if (read(fd, &msgtype, sizeof(msgtype)) != sizeof(msgtype))
        FATAL_ERROR("Failed to read msg header");
    int msg_size = get_message_size(msgtype);
    if (msg_size == -1)
        FATAL_ERROR("Cannot read message of size -1, wrong msg header received");
    void* buf = malloc(msg_size * sizeof(char));
    if (read(fd, buf, msg_size) != msg_size)
        FATAL_ERROR("Failed to read msg body");
    *type = msgtype;
    return buf;
}

void *server_read(int fd, int *type, int *is_closed) {
    int msgtype;
    int total = 0;
    if ((total = read(fd, &msgtype, sizeof(msgtype))) != sizeof(msgtype)) {
        // connection has been closed
        if (total == 0) {
            *is_closed = 1;
            return NULL;
        } else {
            FATAL_ERROR("Failed to read msg header");
        }
    }
    int msg_size = get_message_size(msgtype);
    if (msg_size == -1)
        FATAL_ERROR("Cannot read message of size -1, wrong msg header received");
    void* buf = malloc(msg_size * sizeof(char));
    if (read(fd, buf, msg_size) != msg_size)
        FATAL_ERROR("Failed to read msg body");
    *type = msgtype;
    return buf;

}
