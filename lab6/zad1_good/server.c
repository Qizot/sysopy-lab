#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include "chat.h"

//signatures of available commands
void init(int clientPID, char msg[MAX_MSG_LENGTH]);

void stop(int clientID);

void list(int clientID);

void connect(int clientID, char msg[MAX_MSG_LENGTH]);

void disconnect(int clientID);

void executeCommands(struct msg *msg);

void sendMessage(enum MSG_COMMAND type, char msg[MAX_MSG_LENGTH], int clientID);


int working = 1;
int server_queue = -1;
int active_clients = 0;

typedef struct {
    int client_queue;
    int current_peer;
    int is_active;
    int is_available;
    pid_t pid;
} client_t;

client_t clients[MAX_CLIENTS];

void exitHandler(int signo) {

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_queue != -1) {
            stop(i);
        }
    }

    if (msgctl(server_queue, IPC_RMID, NULL) == -1)
        raise_error("Error while deleting server queue\n");
    else
        printf("\033[1;31mServer:\033[0m Queue has been deleted \n");
    exit(EXIT_SUCCESS);
}

int main() {
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].client_queue = -1;
        clients[i].current_peer = -1;
        clients[i].pid = -1;
        clients[i].is_active = 0;
        clients[i].is_available = 0;
    }
    //handle SIGINT -> delete queue
    struct sigaction act;
    act.sa_handler = exitHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    if ((server_queue = msgget(getServerQueueKey(), IPC_CREAT | 0666)) == -1)
        raise_detailed_error("Error while creating server queue");

    struct msg msgBuff;
    while (working) {
        if (msgrcv(server_queue, &msgBuff, MSGSZ, -COMMAND_TYPES, 0) == -1)
            raise_detailed_error("Error while receiving message");
        executeCommands(&msgBuff);
        usleep(500 * 1000);
    }

    if (msgctl(server_queue, IPC_RMID, NULL) == -1)
        raise_detailed_error("Error while deleting server queue");
    else
        printf("\033[1;31mServer:\033[0m Queue has been deleted\n");

    return 0;
}

void executeCommands(struct msg *msg) {
    printf("\033[1;31mServer:\033[0m server has received new message \n");
    long type = msg->mType;
    switch (type) {
        case STOP:
            stop(msg->sender);
            break;
        case INIT:
            init(msg->sender, msg->msg);
            break;
        case LIST:
            list(msg->sender);
            break;
        case CONNECT:
            connect(msg->sender, msg->msg);
            break;
        case DISCONNECT:
            disconnect(msg->sender);
            break;
        default:
            raise_error("Invalid command");
    }
}

void sendMessage(enum MSG_COMMAND type, char msg[MAX_MSG_LENGTH], int clientID) {
    if (clientID >= MAX_CLIENTS || clientID < 0 || clients[clientID].client_queue < 0) {
        raise_error("Error while sending message to client, client doesn't exist\n");
    }

    struct msg message;
    // sender is useless field in this case
    message.sender = -1;
    message.mType = type;
    strcpy(message.msg, msg);

    if (msgsnd(clients[clientID].client_queue, &message, MSGSZ, IPC_NOWAIT))
        raise_detailed_error("Error while sending message");
}

void init(int clientPID, char msg[MAX_MSG_LENGTH]) {
    int id;
    for (id = 0; id < MAX_CLIENTS; id++) {
        if (clients[id].client_queue == -1)
            break;
    }

    if (id >= MAX_CLIENTS)
        raise_error("Too many clients\n");

    int client_queue = -1;
    sscanf(msg, "%d", &client_queue);
    if (client_queue < 0)
        raise_error("Error while opening client queue \n");

    clients[id].client_queue = client_queue;
    clients[id].pid = clientPID;
    clients[id].current_peer = -1;
    clients[id].is_available = 1;
    clients[id].is_active = 1;

    printf("\033[1;31mServer:\033[0m New client has been added: %d \n", id);

    char toClient[MAX_MSG_LENGTH];
    sprintf(toClient, "%d", id);
    sendMessage(INIT, toClient, id);
    active_clients++;
}

void stop(int clientID) {
    if (clientID >= 0 && clientID < MAX_CLIENTS) {
        char buf[MAX_MSG_LENGTH];
        sendMessage(STOP, buf, clientID);
        if (clients[clientID].pid != -1)
            kill(clients[clientID].pid, SIGRTMIN);

        clients[clientID].client_queue = -1;
        clients[clientID].pid = -1;
        clients[clientID].current_peer = -1;
        clients[clientID].is_available = 0;
        clients[clientID].is_active = 0;
        active_clients--;
    }

}

void list(int clientID) {
    printf("\033[1;31mServer:\033[0m LIST for client: %d \n", clientID);
    char response[MAX_MSG_LENGTH], buf[MAX_MSG_LENGTH];
    strcpy(response, "");
    int i = 0;
    for (; i < MAX_CLIENTS; i++) {
        if (clients[i].client_queue >= 0) {
            sprintf(buf, "ID: %d, AVAILABLE: %d, ACTIVE: %d \n", i, clients[i].is_available, clients[i].is_active);
            strcat(response, buf);
        }
    }
    sendMessage(LIST, response, clientID);
}

void sendConnectMsg(char content[MAX_MSG_LENGTH], int peer_pid, int client_id) {
    struct msg msg;
    msg.mType = CONNECT;
    msg.sender = peer_pid;
    strcpy(msg.msg, content);

    if (msgsnd(clients[client_id].client_queue, &msg, MSGSZ, IPC_NOWAIT) == -1)
        raise_detailed_error("Error while sending connection message about peer");
    if (clients[client_id].pid != -1)
        kill(clients[client_id].pid, SIGRTMIN);
}


void connect(int clientID, char msg[MAX_MSG_LENGTH]) {
    int peer = convert_to_num(msg);
    if (peer == -1) {
        raise_error("Invalid peer id \n");
        return;
    }

    if (peer >= MAX_CLIENTS || peer < 0 || clients[peer].is_active == 0 || clients[peer].is_available == 0) {
        raise_error("Requested peer has not been active and available\n");
        return;
    }
    clients[clientID].is_available = 0;
    clients[peer].is_available = 0;

    clients[clientID].current_peer = peer;
    clients[peer].current_peer = clientID;


    char response[MAX_MSG_LENGTH];
    sprintf(response, "%d", clients[clientID].client_queue);
    sendConnectMsg(response, clients[clientID].pid, peer);

    memset(response, 0, MAX_MSG_LENGTH);
    sprintf(response, "%d", clients[peer].client_queue);
    sendConnectMsg(response, clients[peer].pid, clientID);
}

void disconnect(int clientID) {
    int peer = clients[clientID].current_peer;

    char response[MAX_MSG_LENGTH];

    sendMessage(DISCONNECT, response, clientID);
    int pid = -1;
    if ((pid = clients[clientID].pid) != -1)
        kill(pid, SIGRTMIN);
    sendMessage(DISCONNECT, response, peer);
    if ((pid = clients[peer].pid) != -1)
        kill(pid, SIGRTMIN);

    clients[clientID].current_peer = -1;
    clients[clientID].is_available = 1;
    clients[peer].current_peer = -1;
    clients[peer].is_available = 1;
}



