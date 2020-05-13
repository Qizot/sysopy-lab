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

#define log(format, ...) {printf("\033[1;31m %s: %d %s \033[0m", __FILE__, __LINE__, strerror(errno)); printf(format, ##__VA_ARGS__);}
#define FATAL_ERROR(format, ...) { log(format, ##__VA_ARGS__); printf("\n"); exit(1);}

#define MAX_CLIENT_NAME 80

typedef enum message_type {
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

// gets message's size in bytes
int get_message_size(message_type_t type);


// basic message being meta class for
// obtaining information about message type
typedef struct basic_message {
    message_type_t type;
} basic_message_t;


// used by: server
// message statuses: CONNECTED, DISCONNECTED, WAITING, INFO, PING, PONG
typedef struct info_message {
    message_type_t type;
    UDP_CLIENT
    int success; // contains code for success message
    int error; // contains code for error message, if no error then -1
} info_message_t;

// used by: client
// message statuses: CONNECT, DISCONNECT
typedef struct action_message {
    message_type_t type;
    UDP_CLIENT
    char name[MAX_CLIENT_NAME];
} action_message_t;


// used by: client
// message statuses: MOVE
// field -> value [0-9]
typedef struct move_message {
    message_type_t type;
    UDP_CLIENT
    int field;
} move_message_t;


// used by: server
// message statuses: START_GAME
// character -> value 'X' | 'O'
// DISCLAIMER: player with 'X' should be the first one to make a move
typedef struct start_game_message {
    message_type_t type;
    char character;
} start_game_message_t;




void send_message(int fd, basic_message_t* msg, int size);
void* read_message(int fd, int* type);
void* server_read(int fd, int* type, int* is_closed);



#endif //TCP_MESSAGE_H
