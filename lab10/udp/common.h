//
// Created by jakub on 12.05.2020.
//

#ifndef TCP_MESSAGE_H
#define TCP_MESSAGE_H

#define USE_UDP
#undef USE_UDP

#ifdef USE_UDP
#define UDP_CLIENT int client_id;
#else
#define UDP_CLIENT
#endif
#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#define log(format, ...) {printf("\033[1;31m %s: %d %s \033[0m", __FILE__, __LINE__, strerror(errno)); printf(format, ##__VA_ARGS__);}
#define FATAL_ERROR(format, ...) { log(format, ##__VA_ARGS__); printf("\n"); exit(1);}

#define MAX_CLIENT_NAME 80

typedef enum {
    CONNECT = 1,
    CONNECTED = 2,
    DISCONNECT = 3,
    DISCONNECTED = 4,
    WAITING = 5,
    START_GAME = 6,
    MOVE = 7,
    INFO = 8,
    PING = 9,
    PONG = 10,
    GAME_FINISHED = 11
} message_type_t;


typedef enum {
    NET = 1,
    LOCAL = 2
} socket_type_t;

// gets message's size in bytes
int get_message_size(message_type_t type);


// basic message being meta class for
// obtaining information about message type
typedef struct basic_message {
    message_type_t type;
} basic_message_t;


// used by: server
// message statuses: CONNECTED, DISCONNECTED, WAITING, INFO, PING, PONG
typedef struct message {
    message_type_t type;
    int client_id;
    socket_type_t socket_type;

    // info message
    int success; // contains code for success message
    int error; // contains code for error message, if no error then -1

    // action message
    char name[MAX_CLIENT_NAME];

    // move message
    int field;

    // start_game_message
    char character;
} message_t;

typedef struct client {
    struct sockaddr* sockaddr;
    socklen_t socket_len;
    socket_type_t socket_type;
    char name[MAX_CLIENT_NAME];
    int pings;
    int is_active;
    int current_game;
    int client_id;
} client_t;

void send_message_to(int socket, struct sockaddr* sockaddr, socklen_t  socklen, message_t* msg);
void send_message_to_client(int socket, client_t* client, message_t* msg);

void* read_message(int fd, int* type);
void* server_read(int fd, int* type, int* is_closed);



#endif //TCP_MESSAGE_H
