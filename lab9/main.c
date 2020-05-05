#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX 1000
#define WAS_EMPTY 1
#define WAS_FULL 2
#define WAS_AVAILABLE 3

int k, n;
int* chairs;
int waiting_clients = 0;

// only barber modifies already_cut variable so there is no need for synchronization
int already_cut = 0;
pthread_mutex_t waiting_room_mtx;
pthread_cond_t barber_cond;
int is_barber_sleeping = 0;
int current_client_idx = -1;

pthread_t barber;
pthread_t* clients;


void cut_client(int idx) {
    int sleep_time = 1000 * (rand() % 1000);
    usleep(sleep_time);
    already_cut += 1;
}

int pick_client() {
    int client_idx = -1;
    for (int i = 0; i < k; i++) {
        if (chairs[i] != -1) {
            client_idx = chairs[i];
            chairs[i] = -1;
            waiting_clients -= 1;
            return client_idx;
        }
    }
}

void comeback_later() {
    int sleep_time = 10 * 1000 * (rand() % 1000);
    usleep(sleep_time);
}

void barber_thread(void* _) {
    while (already_cut < n) {
        pthread_mutex_lock(&waiting_room_mtx);
        while (waiting_clients == 0) {
            puts("Golibroda: ide spac");
            is_barber_sleeping = 1;
            pthread_cond_wait(&barber_cond, &waiting_room_mtx);
        }
        is_barber_sleeping = 0;
        // when clients wakes up the barber he is instantly being chosen
        if (current_client_idx == -1) {
            current_client_idx = pick_client();
        }
        int currently_waiting = waiting_clients;
        // we got the client so let others join to waiting room
        pthread_mutex_unlock(&waiting_room_mtx);

        printf("Golibroda: czeka %d klientow, gole klienta: %d\n", currently_waiting, current_client_idx);
        cut_client(current_client_idx);
        current_client_idx = -1;
    }
    printf("Golibroda: koniec na ten dzien, obsluzonych klientow: %d\n", already_cut);
}


int take_place(int idx) {
    if (waiting_clients == 0 && is_barber_sleeping == 1) {
        waiting_clients += 1;
        current_client_idx = idx;
        return WAS_EMPTY;
    }
    if (waiting_clients < k) {
        // we are sure that at least on chair is empty
        for (int i = 0; i < k; i++) {
            if (chairs[i] == -1) {
                chairs[i] = idx;
                waiting_clients += 1;
                return WAS_AVAILABLE;
            }
        }
    }
    return WAS_FULL;
}
void client_thread(void *args) {
    int *arg = (int*) args;
    int idx = arg[0];
    free(args);

    int running = 1;
    while (running > 0) {
        pthread_mutex_lock(&waiting_room_mtx);
        int status = take_place(idx);

        switch (status) {
            case WAS_EMPTY: {
                running = 0;
                printf("Budze golibrode; %d\n", idx);
                pthread_cond_signal(&barber_cond);
                break;
            }
            case WAS_AVAILABLE: {
                running = 0;
                printf("Poczekalnia, wolne miejsca: %d; %d\n", k - waiting_clients, idx);
                break;
            }
            case WAS_FULL: {
                printf("Zajete; %d\n", idx);
                // right now client goes out and waits to come back
                break;
            }
            default: {
                printf("Nieznany kod\n");
                running = 0;
                break;
            }
        }
        pthread_mutex_unlock(&waiting_room_mtx);
        if (status == WAS_FULL)
            comeback_later();
    }

}

int main(int argc, char** argv) {
    if (argc < 3) {
        puts("invalid number of arguments, expected: <number of chairs> <number of clients>");
        exit(EXIT_FAILURE);
    }
    k = atoi(argv[1]);
    n = atoi(argv[2]);
    if (k <= 0 || n <= 0 || k > MAX || n > MAX) {
        puts("either k or n has been invalid, number should be in range <1, 1000>");
        exit(EXIT_FAILURE);
    }
    chairs = malloc(k * sizeof(int));
    for (int i = 0; i < k; i++)
        chairs[i] = -1;

    if (pthread_mutex_init(&waiting_room_mtx, NULL) != 0) {
        puts("failed to initialize mutex");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&barber_cond, NULL) != 0) {
        puts("failed to initialize cond variable");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&barber, NULL, (void *(*)(void *)) barber_thread, NULL) != 0) {
        puts("failed to create barber thread");
        exit(EXIT_FAILURE);
    }

    clients = calloc(n, sizeof(pthread_t));
    for (int i = 0; i < n; i++) {
        int *idx = malloc(sizeof(int));
        *idx = i;
        if (pthread_create(&clients[i], NULL, (void *(*)(void *)) client_thread, (void*) idx) != 0) {
            puts("failed to create client thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < n; i++) {
        if (pthread_join(clients[i], NULL) != 0) {
            puts("failed to join client thread");
            exit(EXIT_FAILURE);
        }
    }
    if (pthread_join(barber, NULL) != 0) {
        puts("failed to join barber thread");
        exit(EXIT_FAILURE);
    }

    return 0;
}
