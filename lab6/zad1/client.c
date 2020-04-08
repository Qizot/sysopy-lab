//
// Created by jakub on 07.04.2020.
//

#include "message.h"
#include "chat_utils.h"
#include <sys/param.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stddef.h>
#include <string.h>
#include <sys/msg.h>


int server_queue = -1;
int client_queue = -1;
int peer_queue = -1;
int current_client_id = -1;
int running = 1;

char msg_buffer[MSG_SIZE];

void clear_buffer() {
    memset(msg_buffer, 0, sizeof(msg_buffer));
}


void close_client_queue() {
    if (client_queue != -1) {
        if (msgctl(client_queue, IPC_RMID, NULL) == -1) {
            perror("failed to close client queue");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
}

void send_to_server(message_type type, char msg[MSG_MAX_SIZE]) {
    message_t message;
    memset(&message, 0, sizeof(message));
    message.sender = current_client_id;
    message.mType = type;
    strcpy(message.msg, msg);
    if (msgsnd(server_queue, &msg, MSG_SIZE, IPC_NOWAIT) == -1) {
        perror("[ERROR] failed to send a message to the server");
    }
}

void get_active_clients() {
    clear_buffer();
    send_to_server(LIST, msg_buffer);
}

void request_connection_with_peer(int peer_id) {
    clear_buffer();
    sprintf(msg_buffer, "%d", peer_id);
    send_to_server(CONNECT, msg_buffer);
}

void send_message_to_peer(char* msg) {
    if (peer_queue == -1) {
        puts("[ERROR] You are currently not connected with any peer...");
        return;
    }
    message_t message;
    message.sender = getpid();
    message.mType = ECHO;
    strcpy(message.msg, msg);
    if (msgsnd(peer_queue, &msg, MSG_SIZE, IPC_NOWAIT)) {
        puts("[ERROR] failed to send message to peer");
    }
}

void disconnect_from_peer() {
    clear_buffer();
    send_to_server(DISCONNECT, msg_buffer);
    peer_queue = -1;
}

void notify_stop() {
    clear_buffer();
    send_to_server(STOP, msg_buffer);
}

void on_message_receive(message_t* msg) {
    switch (msg->mType) {
        case INIT:
            current_client_id = msg->sender;
            puts("Connected with server...");
            break;
        case LIST:
            printf("%s\n", msg->msg);
            break;
        case DISCONNECT:
            peer_queue = -1;
            printf("You have been disconnected from your peer...");
            break;
        case CONNECT:
            peer_queue = msg->sender;
            puts("Connected with peer (now you can only send either ECHO or DISCONNECT messages");
            break;
        case ECHO:
            printf("%s\n", msg->msg);
            break;
        case STOP:
            puts("Closing queue and exiting program...");
            close_client_queue();
            break;
        default:
            puts("[ERROR] unknown message type has been received...");
            break;
    }
}

void message_handler(int signo) {
    puts("Received message...");
    message_t msg;
    if (msgrcv(client_queue, &msg, MSG_SIZE, -7, 0) == -1) {
        puts("[ERROR] failed to handle message receive");
    } else {
        on_message_receive(&msg);
    }
}

void exit_handler(int signo) {
    notify_stop();
    close_client_queue();
}


int starts_with(char* str, char* prefix) {
    if (strncmp(prefix, str, strlen(prefix)) == 0) {
        return 0;
    }
    return -1;
}

void parse_and_execute_command(char* cmd) {
    if (!cmd) return;

//    message_t msg;
//    memset(&msg, 0, sizeof(msg));
//    msg.mType = LIST;
//    msg.sender = current_client_id;
//    if (msgsnd(server_queue, &msg, MSG_SIZE, IPC_NOWAIT) == -1) {
//        perror("[ERROR] failed to INIT with server");
//        exit(EXIT_FAILURE);
//    }
//    return;
   if (starts_with(cmd, "LIST") == 0) {
       get_active_clients();
   } else if (starts_with(cmd, "CONNECT") == 0) {
       char buff[100];
       int peer;
       if (sscanf(cmd, "%s %d", buff, &peer) != 2) {
           puts("\n invalid command format, try again...\n");
           return;
       }
       request_connection_with_peer(peer);
   } else if (starts_with(cmd, "ECHO") == 0) {
       char tmp[100];
       clear_buffer();
       if (sscanf(cmd, "%s %s", tmp, msg_buffer) != 2) {
           puts("\n invalid command format, try again...\n");
           return;
       }
        send_message_to_peer(msg_buffer);
   } else if (starts_with(cmd, "DISCONNECT") == 0) {
       disconnect_from_peer();
   } else if (starts_with(cmd, "STOP") == 0) {
       notify_stop();
   }
}

void init() {
    if ((server_queue = msgget(get_server_queue_key(), 0)) == -1) {
        perror("[ERROR] failed to connect with server's queue");
        exit(EXIT_FAILURE);
    }
    if ((client_queue = msgget(get_client_queue_key(), IPC_CREAT | IPC_EXCL | 0666)) == -1) {
        perror("[ERROR] failed to  create client's queue");
        exit(EXIT_FAILURE);
    }
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.mType = INIT;
    sprintf(msg.msg, "%d", client_queue);
    msg.sender = getpid();
    if (msgsnd(server_queue, &msg, MSG_SIZE, IPC_NOWAIT) == -1) {
        perror("[ERROR] failed to INIT with server");
        exit(EXIT_FAILURE);
    }
}

void main() {
    struct sigaction act;
    act.sa_handler = message_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGRTMIN, &act, NULL);
    signal(SIGINT, exit_handler);

    init();
    for (;;) {
        char input[1024];
        printf("cmd> ");
        fgets(input, sizeof(input), stdin);
        parse_and_execute_command(input);
    }
}



