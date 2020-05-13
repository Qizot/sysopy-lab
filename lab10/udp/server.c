//
// Created by jakub on 12.05.2020.
//
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "error_msg.h"

#define MAX_GAMES 50
#define MAX_CLIENTS MAX_GAMES * 2
#define PING_INTERVAL 5



typedef struct game {
    client_t* client_one;
    client_t* client_two;
    int is_game_on;
} game_t;

game_t games[MAX_GAMES];
client_t clients[MAX_CLIENTS];
int total_clients = 0;



uint16_t port;
char* hpath;

int net_socket;
int local_socket;
int epoll;

pthread_t ping;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void init(char* port, char* path);
void cleanup();
void handle_signal(int so);

_Noreturn void ping_clients(void* _);

void handle_new_client(int fd, message_t* msg, struct sockaddr* sockaddr, socklen_t sock_len);
void handle_new_message(int fd);

void handle_unresponsive_client(client_t* client);

void manage_new_client(int socket, client_t* client);
void disconnect_client(client_t* client);


int main(int argc, char** argv) {
    if (argc != 3)
        FATAL_ERROR("Invalid number of arguments, expected: <port> <path name>")
    if (atexit(cleanup) == -1)
        FATAL_ERROR("Failed to setup atexit hook")
    init(argv[1], argv[2]);

    struct epoll_event event;
    for (;;) {
        if (epoll_wait(epoll, &event, 1, -1) == -1) {
            log("Failed to wait on epoll")
        } else {
            handle_new_message(event.data.fd);
        }
    }

}


void init(char* str_port, char* path) {
    struct sigaction act;
    act.sa_handler = handle_signal;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].is_active = 0;
        clients[i].pings = 0;
    }

    port = (uint16_t)atoi(str_port);
    hpath = path;

    struct sockaddr_in net_address;
    memset(&net_address, 0, sizeof(struct sockaddr_in));
    net_address.sin_family = AF_INET;
    net_address.sin_addr.s_addr = htonl(INADDR_ANY);
    net_address.sin_port = htons(port);

    if ((net_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        FATAL_ERROR("Count not create net socket")
    if (bind(net_socket, &net_address, sizeof(net_address)))
        FATAL_ERROR("Could not bind net socket")

    struct sockaddr_un local_address;
    local_address.sun_family = AF_UNIX;
    strncpy(local_address.sun_path, path, sizeof(local_address.sun_path) - 1);

    if ((local_socket = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
        FATAL_ERROR("Count not create local socket")
    if (bind(local_socket, &local_address, sizeof(local_address))) {
        perror("Reason");
        FATAL_ERROR("Could not bind local socket")
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;
    if ((epoll = epoll_create1(0)) == 1)
        FATAL_ERROR("Failed to create epoll")
    // WARNING: we removed '-' from sockets
    event.data.fd = net_socket;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, net_socket, &event) == -1)
        FATAL_ERROR("Could not add net socket to epoll")
    event.data.fd = local_socket;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, local_socket, &event) == -1)
        FATAL_ERROR("Could not add local socket to epoll")


    if (pthread_create(&ping, NULL, ping_clients, NULL) != 0)
        FATAL_ERROR("Failed to create ping clients thread")
}


void clean_game(game_t* game) {
    game->is_game_on = 0;
    game->client_one = NULL;
    game->client_two = NULL;
}

void cleanup() {
    log("Cleaning up...\n")
    pthread_cancel(ping);


    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].is_active) {
            message_t msg = {
                    .type = DISCONNECTED,
                    .success = 0,
                    .error = -1
            };
            int socket = clients[i].socket_type == NET ? net_socket : local_socket;
            send_message_to_client(socket, &clients[i], &msg);
        }
    }

    if (close(net_socket) == -1)
        log("Failed to close net socket\n")
    if (close(local_socket) == -1)
        log("Failed to close local socket\n")
    if (close(epoll) == -1)
        log("Failed to close epoll\n")
    if (unlink(hpath) == -1)
        log("Failed to unlink hpath\n")
}

void handle_signal(int no) {
    exit(1);
}

_Noreturn void ping_clients(void* _) {
    for (;;) {
        pthread_mutex_lock(&mutex);
        log("Pinging clients...\n")
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].is_active == 0)
                continue;
            if (clients[i].pings > 0) {
                log("Client: '%s' failed to respond, removing client...\n", clients[i].name)

                pthread_mutex_unlock(&mutex);
                handle_unresponsive_client(&clients[i]);
                pthread_mutex_lock(&mutex);
            } else {
                log("Pinging client %s\n", clients[i].name)
                message_t msg = {
                    .type = PING,
                    .success = 0,
                    .error = 0
                };
                int socket = clients[i].socket_type == NET ? net_socket : local_socket;
                send_message_to_client(socket, &clients[i], &msg);
                clients[i].pings += 1;
            }
        }
        pthread_mutex_unlock(&mutex);
        sleep(PING_INTERVAL);
    }
}

void handle_new_client(int fd, message_t* msg, struct sockaddr* sockaddr, socklen_t sock_len) {
    pthread_mutex_lock(&mutex);
    int idx = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].is_active == 0) {
            idx = i;
            break;
        }
    }
    int socket = fd == net_socket ? net_socket : local_socket;
    if (idx == -1) {
        log("Reached max clients\n")
        message_t connect_msg = {
                .type = CONNECT,
                .error = CLIENT_LIMIT_REACHED
        };
        send_message_to(socket, sockaddr, sock_len, &connect_msg);
    } else {
        int is_name_taken = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].is_active == 0)
                continue;
            if (strcmp(clients[i].name, msg->name) == 0) {
                is_name_taken = 1;
                break;
            }
        }


        client_t* client = &clients[idx];
        client->client_id = total_clients++;
        client->sockaddr = malloc(sizeof(struct sockaddr));
        memcpy(client->sockaddr, sockaddr, sock_len);
        client->socket_len = sock_len;
        client->socket_type = fd == net_socket ? NET : LOCAL;
        strcpy(client->name, msg->name);
        client->pings = 0;
        client->is_active = 1;

        // if nickname has already been taken, send message and disconnect client
        if (is_name_taken == 1) {
            message_t connected_msg = {
                    .type = CONNECTED,
                    .error = NAME_TAKEN,
                    .success = -1
            };
            send_message_to_client(socket, client, &connected_msg);
            disconnect_client(client);
            pthread_mutex_unlock(&mutex);
            return;
        }

        message_t connected_msg = {
                .type = CONNECTED,
                .client_id = client->client_id,
                .error = -1,
                .success = 0
        };
        send_message_to_client(socket, client, &connected_msg);
        // connect client to a game
        manage_new_client(socket, client);
    }
    pthread_mutex_unlock(&mutex);
}

void manage_new_client(int socket, client_t* client) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i].is_game_on == 1)
            continue;
        // set current game of client
        client->current_game = i;
        if (games[i].client_one == NULL) {
            games[i].client_one = client;
            message_t msg = {
                    .type = WAITING,
                    .error = -1
            };
            send_message_to_client(socket, client, &msg);
        } else {
            games[i].is_game_on = 1;
            games[i].client_two = client;
            message_t msg = {
                    .type = START_GAME,
                    .character = 'X'
            };

            int psocket = games[i].client_one->socket_type == NET ? net_socket : local_socket;
            send_message_to_client(psocket, games[i].client_one, &msg);
            msg.character = 'O';
            psocket = games[i].client_two->socket_type == NET ? net_socket : local_socket;
            send_message_to_client(psocket, games[i].client_two, &msg);
        }
        return;
    }
}


client_t* find_client_by_id(int client_id) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_id == client_id) {
            pthread_mutex_unlock(&mutex);
            return &clients[i];
        }
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void disconnect_client(client_t* client) {
    if (client == NULL) return;
    message_t msg = {
            .type = DISCONNECTED,
            .error = -1,
            .success = 0
    };

    int socket = client->socket_type == NET ? net_socket : local_socket;
    send_message_to_client(socket, client, &msg);

    client->client_id = -1;
    client->current_game = -1;
    client->is_active = 0;
    client->pings = 0;
    free(client->sockaddr);
    client->sockaddr = NULL;
}



void handle_unresponsive_client(client_t* client) {
    log("Handling unresponsive client: %s\n", client->name)
    pthread_mutex_lock(&mutex);
    int game_id = client->current_game;
    if (games[game_id].is_game_on) {
        // disconnect oponent
        if (games[game_id].client_one->client_id == client->client_id)
            disconnect_client(games[game_id].client_two);
        else
            disconnect_client(games[game_id].client_one);
    }
    clean_game(&games[game_id]);

    client->client_id = -1;
    client->current_game = -1;
    client->is_active = 0;
    client->pings = 0;
    pthread_mutex_unlock(&mutex);
}

void handle_new_message(int socket) {
    struct sockaddr* sockaddr = malloc(sizeof(struct sockaddr));
    socklen_t socklen = sizeof(struct sockaddr);

    message_t msg;
    if (recvfrom(socket, &msg, sizeof(msg), 0, sockaddr, &socklen) != sizeof(msg))
        FATAL_ERROR("Failed to read message")
    client_t* client = find_client_by_id(msg.client_id);

    if (client == NULL && msg.type != CONNECT) {
        FATAL_ERROR("Couldn't find client, it should not happen")
    }

    switch (msg.type) {
        case CONNECT: {
            handle_new_client(socket, &msg, sockaddr, socklen);
            break;
        }
        case DISCONNECT: {
            pthread_mutex_lock(&mutex);
            game_t* current_game = &games[client->current_game];

            if (msg.type != (DISCONNECT & GAME_FINISHED)) {
                if (client->client_id == current_game->client_one->client_id) {
                    disconnect_client(current_game->client_one);
                } else {
                    disconnect_client(current_game->client_two);
                }
            } else {
                disconnect_client(current_game->client_one);
                disconnect_client(current_game->client_two);
            }
            clean_game(current_game);

            pthread_mutex_unlock(&mutex);
            break;
        }
        case MOVE: {
            game_t* current_game = &games[client->current_game];
            client_t* opponent = NULL;

            if (current_game->client_one == client) {
                opponent = current_game->client_two;
            } else {
                opponent = current_game->client_one;
            }

            message_t move_msg = {
                    .type = MOVE,
                    .field = msg.field
            };
            int osocket = opponent->socket_type == NET ? net_socket : local_socket;
            send_message_to_client(osocket, opponent, &move_msg);
            break;
        }
        case PONG: {
            client->pings -=1;
            break;
        }
        default: {
            FATAL_ERROR("handle_new_message: reaching default handler should be not possible: %d", msg.type)
        }
    }
    free(sockaddr);
}







