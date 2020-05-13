//
// Created by jakub on 12.05.2020.
//
#include "common.h"
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
        case CONNECT:
        case DISCONNECT:
        case MOVE:
        case START_GAME:
            return sizeof(message_t);
    }
    return -1;
}



void send_message_to_client(int socket, client_t* client, message_t* msg) {
    send_message_to(socket, client->sockaddr, client->socket_len, msg);
}

void send_message_to(int socket, struct sockaddr *sockaddr, socklen_t socklen, message_t *msg) {
    if (sendto(socket, msg, sizeof(message_t), 0, sockaddr, socklen) != sizeof(message_t))
        FATAL_ERROR("Failed to send message");
}

