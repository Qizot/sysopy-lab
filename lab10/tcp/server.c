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

#include "message.h"
#include "error_msg.h"

#define MAX_GAMES 50
#define MAX_CLIENTS MAX_GAMES * 2


typedef struct client {
    int fd;
    int client_id;
    char name[MAX_CLIENT_NAME];
    int pings;
    int is_active;
    int current_game;
} client_t;

typedef struct game {
    client_t* client_one;
    client_t* client_two;
    int is_game_on;
} game_t;

game_t games[MAX_GAMES];
client_t clients[MAX_CLIENTS];



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

void handle_new_client(int fd);
void handle_new_message(int fd);

void manage_new_client(client_t* client);
void disconnect_client(client_t* client);


int main(int argc, char** argv) {
    if (argc != 3)
        FATAL_ERROR("Invalid number of arguments, expected: <port> <path name>");
    if (atexit(cleanup) == -1)
        FATAL_ERROR("Failed to setup atexit hook");
    init(argv[1], argv[2]);

    struct epoll_event event;
    for (;;) {
        if (epoll_wait(epoll, &event, 1, -1) == -1)
            log("Failed to wait on epoll")
        if (event.data.fd < 0) {
            handle_new_client(-event.data.fd);
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

    if ((net_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        FATAL_ERROR("Count not create net socket")
    if (bind(net_socket, &net_address, sizeof(net_address)))
        FATAL_ERROR("Could not bind net socket")
    if (listen(net_socket, MAX_CLIENTS) == -1)
        FATAL_ERROR("Could not listen on net socket")

    struct sockaddr_un local_address;
    local_address.sun_family = AF_UNIX;
    sprintf(local_address.sun_path, "%s", path);

    if ((local_socket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        FATAL_ERROR("Count not create local socket")
    if (bind(local_socket, &local_address, sizeof(local_address))) {
        perror("Reason");
        FATAL_ERROR("Could not bind local socket")
    }
    if (listen(local_socket, MAX_CLIENTS) == -1)
        FATAL_ERROR("Could not listen on local socket")


    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;
    if ((epoll = epoll_create1(0)) == 1)
        FATAL_ERROR("Failed to create epoll")

    event.data.fd = -net_socket;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, net_socket, &event) == -1)
        FATAL_ERROR("Could not add net socket to epoll")
    event.data.fd = -local_socket;
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
                disconnect_client(&clients[i]);
            } else {
                log("Pinging client %s\n", clients[i].name)
                info_message_t msg = {
                    .type = PING,
                    .success = 0,
                    .error = 0
                };
                send_message(clients[i].fd, &msg, sizeof(msg));
                clients[i].pings += 1;
            }
        }
        pthread_mutex_unlock(&mutex);
        sleep(120);
    }
}

void handle_new_client(int fd) {
    pthread_mutex_lock(&mutex);
    int client_fd = accept(fd, NULL, NULL);
    if (client_fd == -1)
        FATAL_ERROR("Failed to accept client")
    int idx = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].is_active == 0) {
            idx = i;
            break;
        }
    }
    // max clients reached
    if (idx == -1) {
        log("Reached max clients\n")
        info_message_t msg = {
                .type = CONNECT,
                .error = CLIENT_LIMIT_REACHED
        };
        send_message(client_fd, &msg, get_message_size(CONNECT));
        close(client_fd);
    } else {
        struct epoll_event event;
        event.events = EPOLLIN | EPOLLPRI;
        event.data.fd = client_fd;
        if (epoll_ctl(epoll, EPOLL_CTL_ADD, client_fd, &event) == -1)
            FATAL_ERROR("Failed to add client_fd to epoll")

        int msg_type = -1;
        void* buf = read_message(client_fd, &msg_type);
        if (msg_type != CONNECT)
            FATAL_ERROR("Expected CONNECT type")
        action_message_t * msg = buf;

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
        client->is_active = 1;
        client->fd = client_fd;
        strcpy(client->name, msg->name);


        // if nickname has alreadby been taken, send message and disconnect client
        if (is_name_taken == 1) {
            info_message_t connected_msg = {
                    .type = CONNECTED,
                    .error = NAME_TAKEN,
                    .success = -1
            };
            send_message(client->fd, &connected_msg, sizeof(connected_msg));
            disconnect_client(client);
            pthread_mutex_unlock(&mutex);
            return;
        }

        info_message_t connected_msg = {
                .type = CONNECTED,
                .error = -1,
                .success = 0
        };
        send_message(client->fd, &connected_msg, sizeof(connected_msg));
        // connect client to a game
        manage_new_client(client);
    }
    pthread_mutex_unlock(&mutex);
}

void manage_new_client(client_t* client) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (games[i].is_game_on == 1)
            continue;
        // set current game of client
        client->current_game = i;
        if (games[i].client_one == NULL) {
            games[i].client_one = client;
            info_message_t msg = {
                    .type = WAITING,
                    .error = -1
            };
            send_message(client->fd, &msg, sizeof(msg));
        } else {
            games[i].is_game_on = 1;
            games[i].client_two = client;
            start_game_message_t msg = {
                    .type = START_GAME,
                    .character = 'X'
            };
            send_message(games[i].client_one->fd, &msg, sizeof(msg));
            msg.character = 'O';
            send_message(games[i].client_two->fd, &msg, sizeof(msg));
        }
        return;
    }
}


client_t* find_client_by_descriptor(int fd) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == fd) {
            pthread_mutex_unlock(&mutex);
            return &clients[i];
        }
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void disconnect_client(client_t* client) {
    if (client == NULL) return;
    info_message_t  msg = {
            .type = DISCONNECTED,
            .error = -1,
            .success = 0
    };

    int fd = client->fd;
    client->fd = -1;
    client->current_game = -1;
    client->is_active = 0;
    client->pings = 0;

    send_message(fd, &msg, sizeof(msg));

    if (epoll_ctl(epoll, EPOLL_CTL_DEL, fd, NULL) == -1)
        FATAL_ERROR("Failed to delete client_fd from epoll")

    shutdown(fd, SHUT_RDWR);
    close(fd);
}



void handle_unresponsive_client(client_t* client) {
    log("Handling unresponsive client: %s\n", client->name);
    pthread_mutex_lock(&mutex);
    int fd = client->fd;
    int game_id = client->current_game;
    if (games[game_id].is_game_on) {
        // disconnect oponent
        if (games[game_id].client_one->fd == client->fd)
            disconnect_client(games[game_id].client_two);
        else
            disconnect_client(games[game_id].client_one);
    }
    clean_game(&games[game_id]);

    client->fd = -1;
    client->current_game = -1;
    client->is_active = 0;
    client->pings = 0;
    pthread_mutex_unlock(&mutex);

    if (epoll_ctl(epoll, EPOLL_CTL_DEL, fd, NULL) == -1)
        FATAL_ERROR("Failed to delete client_fd from epoll")

    shutdown(fd, SHUT_RDWR);
    close(fd);
}

void handle_new_message(int fd) {
    int type = -1;
    int is_closed = 0;
    void* buf = server_read(fd, &type, &is_closed);
    client_t* client = find_client_by_descriptor(fd);

    if (client == NULL) {
        FATAL_ERROR("Couldn't find client, it should not happen")
    }

    if (is_closed == 1) {
        return handle_unresponsive_client(client);
    }

    switch (type) {
        case DISCONNECT: {
            pthread_mutex_lock(&mutex);
            game_t* current_game = &games[client->current_game];

            action_message_t *msg = buf;
            if (msg->type != (DISCONNECT & GAME_FINISHED)) {
                if (client->fd == current_game->client_one->fd) {
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
            move_message_t* msg = buf;
            if (current_game->client_one == client) {
                send_message(current_game->client_two->fd, msg, sizeof(*msg));
            } else {
                send_message(current_game->client_one->fd, msg, sizeof(*msg));
            }
            break;
        }
        case PONG: {
            client->pings -=1;
            break;
        }
        default: {
            FATAL_ERROR("handle_new_message: reaching default handler should be not possible: %d", type)
        }
    }
    free(buf);
}







