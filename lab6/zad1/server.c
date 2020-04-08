//
// Created by jakub on 07.04.2020.
//
#include "message.h"
#include "chat_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <string.h>

#define MAX_CLIENTS 100

typedef struct {
    int queue;
    int is_active;
    int is_available;
    int current_peer;
    int pid;

} client_t;

client_t clients[MAX_CLIENTS];
int available_idx = 0;
int server_queue = -1;


void send_message(int client_id, message_t* msg) {
    if (msgsnd(clients[client_id].queue, msg, MSG_SIZE, 0) == -1) {
        fprintf(stderr, "[ERROR] failed to send message to client with id: %d\n", client_id);
    } else {
        kill(clients[client_id].pid, SIGRTMIN);
    }
}

void close_server_queue() {
    if (msgctl(server_queue, IPC_RMID, NULL) == -1) {
        perror("[ERROR] failed to close server queue");
        exit(EXIT_FAILURE);
    }
    puts("Closing queue...");
    exit(EXIT_SUCCESS);
}

void exit_handler(int signo) {
    message_t msg;
    msg.mType = STOP;
    for (int i = 0; i < available_idx; i++) {
        send_message(i, &msg);
    }
    close_server_queue();
}


void initialize_client(int client_queue, int pid) {
    int client_id = available_idx++;
    clients[client_id].queue = client_queue;
    clients[client_id].is_available = 1;
    clients[client_id].is_active = 1;
    clients[client_id].current_peer = -1;
    clients[client_id].pid = pid;

    message_t msg;
    msg.mType = INIT;
    msg.sender = client_id;
    send_message(client_id, &msg);
    printf("Added new client with id: %d\n", client_id);
}

char* boolean(int i) {
    return i == 1 ? "true" : "false";
}

void list_active_clients(int client_id) {
    char buffer[MSG_MAX_SIZE] = {0};
    for (int i = 0; i < available_idx; i++) {
        if (i == client_id)
            continue;
        char tmp[100];
        sprintf(tmp, "ID: %d -> AVAILABLE: %s, ONLINE: %s\n", i, boolean(clients[i].is_available), boolean(clients[i].is_active));
        strcat(buffer, tmp);
    }
    message_t msg;
    msg.mType = LIST;
    strcpy(msg.msg, buffer);
   send_message(client_id, &msg);
   printf("Listing all active clients for client: %d\n", client_id);
}

void connect_client(int client_id, int peer_id) {
   if (clients[peer_id].is_available == 0 || clients[peer_id].is_active == 0) {
       return;
   }
   clients[peer_id].is_available = 0;
   clients[peer_id].current_peer = client_id;
   clients[client_id].is_available = 0;
   clients[client_id].current_peer = peer_id;

   message_t msg;
   msg.mType = CONNECT;

   msg.sender = clients[peer_id].queue;
   send_message(client_id, &msg);

   msg.sender = clients[client_id].queue;
   send_message(peer_id, &msg);
   printf("Connecting client: %d with peer: %d\n", client_id, peer_id);
}

void disconnect_client(int client_id) {
    message_t msg;
    msg.mType = DISCONNECT;

    int peer = clients[client_id].current_peer;

    clients[peer].current_peer = -1;
    clients[peer].is_available = 1;
    clients[client_id].current_peer = -1;
    clients[client_id].is_available = 1;

    send_message(client_id, &msg);
    send_message(peer, &msg);
    printf("Disconnecting client: %d\n", client_id);
}

void stop_client(int client_id) {
    message_t msg;
    if (clients[client_id].current_peer != -1) {
        msg.mType = DISCONNECT;
        send_message(clients[client_id].current_peer, &msg);
    }
    msg.mType = STOP;
    send_message(client_id, &msg);
    printf("Stopping client: %d\n", client_id);
}

void on_message_receive(message_t* msg) {
    int peer;
    switch (msg->mType) {
        case INIT: {
            int queue_id = parse_number(msg->msg);
            initialize_client(queue_id, msg->sender);
            break;

        }
        case LIST:
            list_active_clients(msg->sender);
        case CONNECT:
            peer = parse_number(msg->msg);
            connect_client(msg->sender, peer);
            break;
        case DISCONNECT:
            disconnect_client(msg->sender);
            break;
        case STOP:
            stop_client(msg->sender);
            break;
        default:
            puts("[ERROR] unknown message type has been received...");
            break;
    }
}

void init() {
    if ((server_queue = msgget(get_server_queue_key(), IPC_CREAT | IPC_EXCL | 0666)) == -1) {
        perror("[ERROR] failed to create server queue");
        exit(EXIT_FAILURE);
    }
}

void main() {
    init();

    struct sigaction act;
    act.sa_handler = exit_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    for (;;) {
        message_t msg;
        if (msgrcv(server_queue, &msg, MSG_SIZE, 0, 0) ==  -1) {
            // puts("[ERROR] failed to receive message");
        } else {
            puts("Received message...");
            on_message_receive(&msg);
        }
        usleep(500 * 1000);
    }
}


