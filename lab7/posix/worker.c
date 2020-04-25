//
// Created by jakub on 21.04.2020.
//

#include <sys/types.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include "delivery_shop.h"
#include "utils.h"

#define N_MAX 10000

delivery_shop_t* shop = NULL;
int shm_id = -1;
sem_t* shop_sem = NULL;

#define WAIT_SEMAPHORE(sem)  \
    if (sem_wait(sem) == -1) \
        FAILURE_EXIT("Failed to wait on semaphore")


#define RELEASE_SEMAPHORE(sem) \
    if (sem_post(sem) == -1) \
        FAILURE_EXIT("Failed to release semaphore")

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

void prepare_fifo() {
    shm_id = shm_open(POSIX_SHOP, O_RDWR, 0666);
    if (shm_id == -1)
        FAILURE_EXIT("Failed to shmget");

    void *tmp = mmap(NULL, sizeof(delivery_shop_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
    if (tmp == (void*)(-1))
        FAILURE_EXIT("Failed to shmat");
    shop = (delivery_shop_t*) tmp;
}

void prepare_semaphores() {
    shop_sem = sem_open(POSIX_SHOP_SEM, O_RDWR);
    if (shop_sem == SEM_FAILED)
        FAILURE_EXIT("Failed to open sem\n");
}

void clean() {

    if (munmap(shop, sizeof(delivery_shop_t)) == -1)
        FAILURE_EXIT("Failed to disconnect shared shop memory");

    if (shop_sem != NULL && sem_close(shop_sem) == -1)
        FAILURE_EXIT("Failed to close sem\n");
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

        WAIT_SEMAPHORE(shop_sem)

        order_t order;
        order.n = n;
        if (push_order(shop, order) != -1) {
            char* timestr = get_current_time();
            printf("[PID: %d][%s] Dodalem liczbe: %d. Liczba zamowien do przygotowania: %d. Liczba zamowien do wyslania: %d\n",
                   getpid(), timestr, n, shop->created, shop->prepared);
            free(timestr);
        }

        RELEASE_SEMAPHORE(shop_sem)
    }
}

int packer() {
    while (1) {
        WAIT_SEMAPHORE(shop_sem)

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

        RELEASE_SEMAPHORE(shop_sem)
    }
}

int sender() {
    while (1) {
        WAIT_SEMAPHORE(shop_sem)

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

        RELEASE_SEMAPHORE(shop_sem)
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
