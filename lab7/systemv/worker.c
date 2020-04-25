//
// Created by jakub on 21.04.2020.
//

#include "delivery_shop.h"
#include "utils.h"
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>

#define N_MAX 10000
#define INCREMENT 1
#define DECREMENT -1

delivery_shop_t* shop = NULL;
key_t fifo_key = -1;
int shm_id = -1;
int sem_id = -1;
int locked[4] = {0};

#define WAIT_SEMAPHORE(sops)  \
    sops.sem_op = DECREMENT; \
    if (semop(sem_id, &sops, 1) == -1) \
        FAILURE_EXIT("Failed to wait on semaphore: sem_num: %d, sem_op: %d", sops.sem_num, sops.sem_op) \
    else \
        locked[sops.sem_num] = 1;


#define RELEASE_SEMAPHORE(sops) \
    sops.sem_op = INCREMENT; \
    if (semop(sem_id, &sops, 1) == -1) \
        FAILURE_EXIT("Failed to release semaphore: sem_num: %d, sem_op: %d", sops.sem_num, sops.sem_op) \
    else \
        locked[sops.sem_num] = 0;

int parse_worker_type(char* name) {
    if (strcmp(name, "receiver") == 0) {
        return RECEIVER;
    }
    if (strcmp(name, "packer") == 0) {
        return PACKER;
    }
    if (strcmp(name, "sender") == 0) {
        return SENDER;
    }
    return -1;
}

void prepare_key() {
    fifo_key = get_shop_key();
}

void prepare_fifo() {
    shm_id = shmget(fifo_key, 0, 0);
    if (shm_id == -1)
        FAILURE_EXIT("Failed to shmget");

    void *tmp = shmat(shm_id, NULL, 0);
    if (tmp == (void*)(-1))
        FAILURE_EXIT("Failed to shmat");
    shop = (delivery_shop_t*) tmp;
}

void prepare_semaphores() {
    sem_id = semget(fifo_key, 0, 0);
    if (sem_id == -1)
        FAILURE_EXIT("Failed to semget");
}

void clean() {
    if (shmdt(shop) == -1)
        log("Failed to disconnect shared memory\n");
    log("Cleaned worker...\n");
}

void int_handler(int signo) {
    exit(1);
}

int receiver() {
    int iterations = 0;
    while (1) {
        iterations++;
        int n = rand() % N_MAX;

        struct sembuf sops;
        sops.sem_num = SHOP;
        sops.sem_flg = 0;

        WAIT_SEMAPHORE(sops)

        order_t order;
        order.n = n;
        if (push_order(shop, order) != -1) {
            char* timestr = get_current_time();
            printf("[PID: %d][%s] Dodalem liczbe: %d. Liczba zamowien do przygotowania: %d. Liczba zamowien do wyslania: %d\n",
                   getpid(), timestr, n, shop->created, shop->prepared);
            free(timestr);
        }

        RELEASE_SEMAPHORE(sops)
    }
}

int packer() {
    while (1) {
        struct sembuf sops;
        sops.sem_num = SHOP;
        sops.sem_flg = 0;

        WAIT_SEMAPHORE(sops)

        order_t* order = calloc(1, sizeof(order_t));
        int idx = -1;
        if ((idx = get_order(shop, order, CREATED)) >= 0) {
            int n = order->n * 2;
            prepare_order(shop, idx);
            char* timestr = get_current_time();

            printf("[PID: %d][%s] Przygotowalem zamowienie o wielkosci: %d. Liczba zamowien do przygotowania: %d. Liczba zamowien do wyslania: %d\n",
                   getpid(), timestr, n, shop->created, shop->prepared);
            free(timestr);
        }
        free(order);

        RELEASE_SEMAPHORE(sops)
    }
}

int sender() {
    while (1) {
        struct sembuf sops;
        sops.sem_num = SHOP;
        sops.sem_flg = 0;

        WAIT_SEMAPHORE(sops)

        order_t* order = calloc(1, sizeof(order_t));
        int idx = -1;
        if ((idx = get_order(shop, order, PREPARED)) >= 0) {
            int n = order->n * 3;
            send_order(shop, idx);
            char* timestr = get_current_time();

            printf("[PID: %d][%s] Wyslalem zamowienie o wielkosci: %d. Liczba zamowien do przygotowania: %d. Liczba zamowien do wyslania: %d\n",
                   getpid(), timestr, n, shop->created, shop->prepared);
            free(timestr);
        }
        free(order);

        RELEASE_SEMAPHORE(sops)
    }
}

int main(int argc, char** argv) {
    if (argc < 2)
        FAILURE_EXIT("Not enought number of arguments in worker: <receiver> | <packer> | <sender>");

    if (atexit(clean) == -1)
        FAILURE_EXIT("Failed to set cleanup handler");
    if (signal(SIGINT, int_handler) == SIG_ERR)
        FAILURE_EXIT("Failed to set sigint handler");

    int worker = parse_worker_type(argv[1]);

    prepare_key();
    prepare_fifo();
    prepare_semaphores();

    switch (worker) {
        case RECEIVER:
            return receiver();
        case PACKER:
            return packer();
        case SENDER:
            return sender();
        default:
            FAILURE_EXIT("Invalid worker type, should be one of: <receiver> | <packer> | <sender>");
    }

}
