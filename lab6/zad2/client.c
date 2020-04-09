#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <mqueue.h>
#include <fcntl.h>
#include "chat.h"

int server_queue = -1;
int client_queue = -1;
int peer_queue = -1;
int peer_pid = -1;
int client_id = -1;
char *queue_name = NULL;
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
    char buffer[MAX_MSG_LENGTH];
    if (mq_receive(client_queue, buffer, MAX_MSG_LENGTH, NULL) == -1)
        raise_detailed_error("Error while receiving message");

    struct msg* msg = parse_to_msg(buffer);
    if (msg == NULL) {
        raise_error("Failed to parse message\n");
    }

    switch (msg->mType) {
        case STOP: {
            exit(EXIT_SUCCESS);
        }
        case CONNECT: {
            peer_queue = mq_open(msg->msg, O_WRONLY);
            if (peer_queue == -1) {
                raise_detailed_error("Failed to connect to peer queue");
            }
            peer_pid = msg->sender;
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
            printf("Peer: %s\n", msg->msg);
            break;
        }
    }
    free(msg);

}

void exit_handler(int signo) {
    puts("SIGINT signal has been received");
    stop();
}

void exit_cleanup() {
    if (mq_close(client_queue) == -1)
        raise_detailed_error("Error while closing client queue");
    if (mq_close(server_queue) == -1)
        raise_detailed_error("Error while closing server queue");
    if (mq_unlink(queue_name) == -1)
        raise_detailed_error("Error while unlinking client queue");

    printf("\033[1;35mClient:\033[0m Queue has been closed and  deleted\n");
}

void parse_and_execute_command(char* cmd);


int main() {
    struct sigaction act;
    act.sa_handler = communicationHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGRTMIN, &act, NULL);

    signal(SIGINT, exit_handler);
    atexit(exit_cleanup);

    queue_name = get_client_queue_name();

    if ((server_queue = mq_open(SERVER_NAME, O_WRONLY)) == -1)
        raise_detailed_error("Error while opening server queue");

    struct mq_attr attrs;
    attrs.mq_maxmsg = MAX_MESSAGES;
    attrs.mq_msgsize = MAX_MSG_LENGTH;

    if ((client_queue = mq_open(queue_name, O_RDONLY | O_CREAT | O_EXCL, 0666, &attrs)) == -1)
        raise_detailed_error("Error while creating client queue");

    init();
    printf("\033[1;35mClient:\033[0m Server's queue ID:\t%d \n", server_queue);
    printf("\033[1;35mClient:\033[0m Client's queue name:\t%s \n", queue_name);

    char line[1024];
    while(1) {
        if (fgets(line, sizeof(line), stdin) == NULL)
            continue;
        parse_and_execute_command(line);
    }
}


void send(enum MSG_COMMAND type, char content[MAX_MSG_LENGTH]) {
    struct msg msg;
    msg.mType = type;
    strcpy(msg.msg, content);
    msg.sender = client_id;

    char* buffer = parse_msg_to_string(&msg);
    if (mq_send(server_queue, buffer, MAX_MSG_LENGTH, get_cmd_priority(type)) == -1)
        raise_detailed_error("Error while sending message to server");
}

void init() {
    struct msg msg;
    char content[MAX_MSG_LENGTH];
    msg.mType = INIT;
    sprintf(content, "%s", queue_name);
    strcpy(msg.msg, content);
    msg.sender = getpid();

    char* buffer = parse_msg_to_string(&msg);
    if (mq_send(server_queue, buffer, MAX_MSG_LENGTH, get_cmd_priority(INIT)) == -1)
        raise_detailed_error("Error while sending message to server");
    free(buffer);

    char response[MAX_MSG_LENGTH];
    if (mq_receive(client_queue, response, MAX_MSG_LENGTH, NULL) == -1)
        raise_error("Failed to connect with server \n");

    struct msg* response_msg = parse_to_msg(response);
    if (response_msg == NULL)
        raise_error("Failed to read INIT message");

    if (response_msg->mType != INIT)
        raise_error("Expected INIT type \n");
    sscanf(response_msg->msg, "%d", &client_id);
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

    char* buffer = parse_msg_to_string(&msg);
    if (mq_send(peer_queue, buffer, MAX_MSG_LENGTH, get_cmd_priority(ECHO)) == -1)
        raise_detailed_error("Error while sending message to peer");
    if (peer_pid != -1) {
        kill(peer_pid, SIGRTMIN);
    }
    free(buffer);

}

void list() {
    send(LIST, "");
    char buffer[MAX_MSG_LENGTH];
    if (mq_receive(client_queue, buffer, MAX_MSG_LENGTH, NULL) == -1)
        raise_error("Failed to get LIST \n");

    struct msg* msg = parse_to_msg(buffer);

    if (msg->mType != LIST)
        raise_error("Expected LIST type \n");

    printf("\033[1;35mClient:\033[0m List of active clients: \n %s \n", msg->msg);
    free(msg);
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



