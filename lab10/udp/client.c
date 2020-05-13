//
// Created by jakub on 12.05.2020.
//

#include "common.h"
#include "error_msg.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

char* client_name;
int client_socket;
int client_id = -1;
socket_type_t socket_type;
struct sockaddr_un* server_address = NULL;

char board[9];
char client_character;
char opponent_character;
int is_clients_move = 0;

pthread_mutex_t mutex;
pthread_cond_t cond;
pthread_t game_thread;

void init(char* name, char* connection_type, char* server_addr, char* port);
void handle_signal(int no);

_Noreturn void handle_messages();
void send_message(message_t* msg);
void start_game(void* _);

void cleanup();

int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) {
        FATAL_ERROR("invalid number of arguments, expected: <client name> <'LOCAL' | 'NET'> <server address> <server port>");
    }
    if (pthread_mutex_init(&mutex, NULL) != 0)
        FATAL_ERROR("Failed to init mutex")
    init(argv[1], argv[2], argv[3], argc == 5 ? argv[4] : NULL);
    atexit(cleanup);

    handle_messages();
}


void init(char* name, char* connection_type, char* server_addr, char* server_port) {
    struct sigaction act;
    act.sa_handler = handle_signal;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    client_name = name;

    int conn_type;
    if (strcmp(connection_type, "NET") == 0) {
        socket_type = NET;
        uint32_t ip = inet_addr(server_addr);
        uint16_t port = atoi(server_port);
        if (port < 1024) {
            FATAL_ERROR("invalid port number, should be greater than 1024")
        }

        struct sockaddr_in net_address;
        memset(&net_address, 0, sizeof(struct sockaddr_in));
        net_address.sin_family = AF_INET;
        net_address.sin_addr.s_addr = htonl(INADDR_ANY);
        net_address.sin_port = htons(port);

        if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            FATAL_ERROR("Failed to create client socker")
        if (connect(client_socket, &net_address, sizeof(net_address)) == -1)
            FATAL_ERROR("Failed to connect with server")
        log("Connected with remote server\n")

    } else if (strcmp(connection_type, "LOCAL") == 0) {
        socket_type = LOCAL;

        server_address = malloc(sizeof(struct sockaddr_un));
        memset(server_address, 0, sizeof(server_address));
        server_address->sun_family = AF_UNIX;
        strncpy(server_address->sun_path, server_addr, sizeof(server_address->sun_path) -1);

        struct sockaddr_un client_address;
        memset(&client_address, 0, sizeof(client_address));
        client_address.sun_family = AF_UNIX;
        snprintf(client_address.sun_path, sizeof(client_address.sun_path) - 1,
                 "/tmp/tic.%ld", (long) getpid());

        if ((client_socket = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
            FATAL_ERROR("Failed to create unix socket")

        if (bind(client_socket, &client_address, sizeof(client_address)) < 0)
            FATAL_ERROR("failed to bind to client address")

//        if (connect(client_socket, server_address, sizeof(*server_address)) == -1)
//            FATAL_ERROR("Failed to connect with server")
        log("Connected with server via local socket")
    } else {
        FATAL_ERROR("invalid connection type")
    }

    message_t msg = {
           .type = CONNECT,
           .socket_type = socket_type,
    };
    strncpy(msg.name, name, MAX_CLIENT_NAME - 1);
    send_message(&msg);
}


void handle_signal(int no) {
    exit(1);
}


_Noreturn void handle_messages() {
    for(;;) {
        message_t msg;
        if (read(client_socket, &msg, sizeof(message_t)) != sizeof(msg))
            FATAL_ERROR("Failed to read message")

        pthread_mutex_lock(&mutex);
        switch (msg.type) {
            case WAITING: {
                puts("Waiting for oponnent...");
                break;
            }
            case START_GAME: {
                client_character = msg.character;
                opponent_character = msg.character == 'X' ? 'O' : 'X';

                if (client_character == 'O')
                    is_clients_move = 1;
                else
                    is_clients_move = 0;
                for (int i = 0; i < 9; i++) {
                   board[i] = ' ' ;
                }
                puts("Starting game...");
                if (is_clients_move == 0)
                    puts("Waiting for oponent to make first move");
                if (pthread_cond_init(&cond, NULL) != 0)
                    FATAL_ERROR("Failed to init cond variable")
                if (pthread_create(&game_thread, NULL, start_game, NULL) != 0)
                    FATAL_ERROR("Failed to start game thread");

                break;
            }
            case DISCONNECTED: {
                FATAL_ERROR("Disconnected by server");
            }
            case MOVE: {
                board[msg.field] = opponent_character;
                is_clients_move = 1;
                pthread_cond_signal(&cond);
                break;
            }
            case PING: {
                message_t msg = {
                        .type = PONG
                };
                send_message(&msg);
                break;
            }
            case CONNECTED: {
                if (msg.error != -1) {
                    switch (msg.error) {
                        case NAME_TAKEN:
                            log("Requested client name has already been taken\n")
                            break;
                        default:
                            break;
                    }
                } else {
                    client_id = msg.client_id;
                    log("Server has confirmed connection\n")
                }
                break;
            }
            default: {
                FATAL_ERROR("Received unknown message type: %d", msg.type);
            }
        }
        pthread_mutex_unlock(&mutex);
    }
}

void draw_board() {

    printf("\n\n");
    printf("%c | %c | %c \n", board[0], board[1], board[2]);
    printf("----------\n");
    printf("%c | %c | %c \n", board[3], board[4], board[5]);
    printf("----------\n");
    printf("%c | %c | %c \n", board[6], board[7], board[8]);

}

#define WON 1
#define LOST 2
#define TIE 3


int is_game_finished() {
    int row = 3;
    for (int i = 0; i < 3; i++) {
        char winner = ' ';
        if (board[row * i] != ' ' && board[row * i] == board[row * i + 1] && board[row * i + 1] == board[row * i + 2]) {
            winner = board[i];
        }
        if (board[i] != ' ' && board[i] == board[i + 3] && board[i + 3] == board[i + 6]) {
            winner = board[i];
        }
        if (winner != ' ') {
            if (winner == client_character) {
                return WON;
            } else {
                return LOST;
            }
        }
    }
    char winner = ' ';
    if (board[0] != ' ' && board[0] == board[4] && board[4] == board[8])
        winner = board[0];
    if (board[2] != ' ' && board[2] == board[4] && board[4] == board[6])
        winner = board[2];
    if (winner != ' ') {
        if (winner == client_character) {
            return WON;
        } else {
            return LOST;
        }
    }
    int all_taken = 1;
    for (int i = 0; i < 9; i++)
        if (board[i] == ' ') {
            all_taken = 0;
            break;
        }
    return all_taken == 1 ? TIE : -1;
}


void end_message(int result) {
    switch (result) {
        case WON:
            puts("You have won!");
            break;
        case LOST:
            puts("You have lost!");
            break;
        case TIE:
            puts("You have tied!");
            break;
    }
}


void start_game(void* _) {
    for (;;) {
        pthread_mutex_lock(&mutex);
        while (is_clients_move == 0) {
            pthread_cond_wait(&cond, &mutex);
        }


        int result = -1;
        draw_board();
        if ((result = is_game_finished()) != -1) {
            end_message(result);

            message_t action = {
                    .type = DISCONNECT & GAME_FINISHED
            };
            send_message(&action);
            pthread_mutex_unlock(&mutex);
            return;
        }


        // important to release mutex for taking input
        pthread_mutex_unlock(&mutex);

        printf("your character is '%c', chose field [0-8]> ", client_character);

        int field = -1;
        do {
            if (field != -1) {
                printf("please chose valid and empty field> ");
            }
            scanf("%d", &field);
        } while (field < 0 || field > 8 || board[field] != ' ');

        pthread_mutex_lock(&mutex);

        board[field] = client_character;
        message_t msg = {
                .type = MOVE,
                .field = field
        };
        send_message(&msg);

        if ((result = is_game_finished()) != -1) {
            draw_board();
            end_message(result);

            message_t action = {
                    .type = DISCONNECT & GAME_FINISHED
            };
            send_message(&action);
            pthread_mutex_unlock(&mutex);
            return;
        }
        is_clients_move = 0;
        puts("Waiting for oponent to make next move...");
        pthread_mutex_unlock(&mutex);
    }
}

void disconnect_from_server() {

}

void cleanup() {
    if (shutdown(client_socket, SHUT_RDWR) == -1) {
        log("Failed to shutdown socket\n");
    }
    if (close(client_socket) == -1) {
        log("Failed to close socket\n");
    }
}

void send_message(message_t *msg) {
    msg->client_id = client_id;
    msg->socket_type = socket_type;
    strcpy(msg->name, client_name);
    if (server_address != NULL) {
        if (sendto(client_socket, msg, sizeof(message_t), 0, server_address, sizeof(*server_address)) != sizeof(message_t))
            FATAL_ERROR("Failed to send message via local socket")
    } else {
        if (write(client_socket, msg, sizeof(message_t)) != sizeof(message_t))
            FATAL_ERROR("failed to send message via net socket")
    }
}
