//
// Created by przjab98 on 27.04.19.
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include "chat.h"

int server_queue = -1;
int client_queue = -1;
int peer_queue = -1;
int peer_pid = -1;
int client_id = -1;
int working = 1;

//commands handlers
void send(enum MSG_COMMAND type, char content[MAX_MSG_LENGTH]);

void echo(char content[MAX_MSG_LENGTH]);

void list();

void stop();

void connect(char arguments[MAX_MSG_LENGTH]);

void disconnect();

void init();

//signal handlers
void communicationHandler(int signo) {
    struct msg msg;
    if (msgrcv(client_queue, &msg, MSGSZ, -COMMAND_TYPES, 0) == -1)
        raise_detailed_error("Error while receiveing message");

    switch (msg.mType) {
        case STOP: {
            exit(EXIT_SUCCESS);
        }
        case CONNECT: {
            peer_queue = convert_to_num(msg.msg);
            peer_pid = msg.sender;
            printf("\033[1;35mClient:\033[0m Connected with peer queue ID:\t%d \n", peer_queue);
            break;
        }
        case DISCONNECT: {
            peer_queue = -1;
            peer_pid = -1;
            printf("\033[1;35mClient:\033[0m Disconnected from peer \n");
            break;
        }
        case ECHO: {
            printf("Peer: %s\n", msg.msg);
            break;
        }
    }

}

void _exit(int signo) {
    puts("SIGINT signal has been received");
    stop();
}

void exit_cleanup() {
    if (msgctl(client_queue, IPC_RMID, NULL) == -1)
        raise_detailed_error("Error while deleting queue");
    else {
        printf("\033[1;35mClient:\033[0m Queue has been deleted\n");
    }
}

void parse_and_execute_command(char* cmd);


int main() {
    struct sigaction act;
    act.sa_handler = communicationHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGRTMIN, &act, NULL);

    signal(SIGINT, _exit);
    atexit(exit_cleanup);

    if ((server_queue = msgget(getServerQueueKey(), 0)) == -1)
        raise_detailed_error("Error while opening server queue");
    if ((client_queue = msgget(getClientQueueKey(), IPC_CREAT | IPC_EXCL | 0666)) == -1)
        raise_detailed_error("Error while creating client queue");

    init();
    printf("\033[1;35mClient:\033[0m Server's queue ID:\t%d \n", server_queue);

    char line[1024];
    while(1) {
        if (fgets(line, sizeof(line), stdin) == NULL)
            continue;
        parse_and_execute_command(line);
    }
    puts("KURWA MAC");
    if (msgctl(client_queue, IPC_RMID, NULL) == -1)
        raise_detailed_error("Error while deleting queue");
    else
        printf("\033[1;35mClient:\033[0m Queue has been deleted \n");

    return 0;
}


void send(enum MSG_COMMAND type, char content[MAX_MSG_LENGTH]) {
    struct msg msg;
    msg.mType = type;
    strcpy(msg.msg, content);
    msg.sender = client_id;
    //printf("what I sent: %s \n", msg.msg);
    if (msgsnd(server_queue, &msg, MSGSZ, IPC_NOWAIT) == -1)
        raise_detailed_error("Error while sending message to server");
}

void init() {
    struct msg msg;
    char content[MAX_MSG_LENGTH];
    msg.mType = INIT;
    sprintf(content, "%i", client_queue);
    strcpy(msg.msg, content);
    msg.sender = getpid();
    if (msgsnd(server_queue, &msg, MSGSZ, IPC_NOWAIT) == -1)
        raise_detailed_error("Error while sending message to server");

    if (msgrcv(client_queue, &msg, MSGSZ, -COMMAND_TYPES, 0) == -1)
        raise_error("Failed to connect with server \n");

    if (msg.mType != INIT)
        raise_error("Expected INIT type \n");
    sscanf(msg.msg, "%d", &client_id);
    printf("\033[1;35mClient:\033[0m Client has got ID: %d \n", client_id);
}


void connect(char content[MAX_MSG_LENGTH]) {
    send(CONNECT, content);
}

void disconnect() {
    char buf[MAX_MSG_LENGTH];
    send(DISCONNECT, buf);
}

void echo(char content[MAX_MSG_LENGTH]) {
    if (peer_queue == -1) {
        raise_error("Error, you have to be connected with peer first! \n");
        return;
    }

    struct msg msg;
    msg.mType = ECHO;
    strcpy(msg.msg, content);
    msg.sender = client_id;
    if (msgsnd(peer_queue, &msg, MSGSZ, IPC_NOWAIT) == -1)
        raise_detailed_error("Error while sending message to peer");
    if (peer_pid != -1) {
        kill(peer_pid, SIGRTMIN);
    }

}

void list() {
    send(LIST, "");
    struct msg msg;
    if (msgrcv(client_queue, &msg, MSGSZ, -COMMAND_TYPES, 0) == -1)
        raise_error("Blad komendy LIST \n");

    if (msg.mType != LIST)
        raise_error("Expected LIST type \n");

    printf("\033[1;35mClient:\033[0m List of active clients: \n %s \n", msg.msg);
}

void stop() {
//    working = 0;
    send(STOP, "");
}

int starts_with(char* str, char* prefix) {
    if (strncmp(prefix, str, strlen(prefix)) == 0) {
        return 0;
    }
    return -1;
}

void parse_and_execute_command(char* cmd) {
    if (!cmd) return;

    if (starts_with(cmd, "LIST") == 0) {
        list();
    } else if (starts_with(cmd, "CONNECT") == 0) {
        char tmp[100];
        char peer[MAX_MSG_LENGTH];
        if (sscanf(cmd, "%s %s", tmp, peer) != 2) {
            puts("\n invalid command format, try again...\n");
            return;
        }
        connect(peer);
    } else if (starts_with(cmd, "ECHO") == 0) {
        char buffer[MAX_MSG_LENGTH];
        strcpy(buffer, cmd);
        echo(buffer);
    } else if (starts_with(cmd, "DISCONNECT") == 0) {
        disconnect();
    } else if (starts_with(cmd, "STOP") == 0) {
        stop();
    }
}



