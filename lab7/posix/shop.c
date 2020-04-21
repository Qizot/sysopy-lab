//
// Created by jakub on 21.04.2020.
//

#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include "delivery_shop.h"
#include "utils.h"

key_t fifo_key = -1;
int shm_id = -1;
int sem_id = -1;
delivery_shop_t* shop = NULL;


sem_t* shop_sem = NULL;
sem_t* receiver_sem = NULL;
sem_t* packer_sem = NULL;
sem_t* sender_sem = NULL;
int children_num = 0;
int *children = NULL;

void create_fifo() {
    shm_id = shm_open(POSIX_SHOP, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_id == -1)
       FAILURE_EXIT("Failed to shm_open");

    if (ftruncate(shm_id, sizeof(delivery_shop_t)) == -1)
       FAILURE_EXIT("Failed to ftruncate");

    void *tmp = mmap(NULL, sizeof(delivery_shop_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
    if (tmp == (void*)(-1))
       FAILURE_EXIT("Failed to mmap");
    shop = (delivery_shop_t*) tmp;
    init_delivery_shop(shop);
}

void create_semaphores() {
    shop_sem = sem_open(POSIX_SHOP_SEM, O_CREAT | O_EXCL, 0666, 1);
    if (shop_sem == SEM_FAILED)
        FAILURE_EXIT("Failed to create shop_sem");

    receiver_sem = sem_open(POSIX_RECEIVER_SEM, O_CREAT | O_EXCL, 0666, 1);
    if (receiver_sem == SEM_FAILED)
        FAILURE_EXIT("Failed to create receiver_sem");

    packer_sem = sem_open(POSIX_PACKER_SEM, O_CREAT | O_EXCL, 0666, 1);
    if (packer_sem == SEM_FAILED)
        FAILURE_EXIT("Failed to create packer_sem");

    sender_sem = sem_open(POSIX_SENDER_SEM, O_CREAT | O_EXCL, 0666, 1);
    if (sender_sem == SEM_FAILED)
        FAILURE_EXIT("Failed to create sender_sem ");
}


void clean() {
    log("Cleaning shop resources...\n");
    for (int i = 0; i < children_num; i++) {
        if (children[i] > 0) {
            kill(children[i], SIGINT);
        }
    }
    if (munmap(shop, sizeof(delivery_shop_t)) == -1)
        log("Failed to disconnect shared memory of shop\n")
    else
        log("Disconnected shared memory\n");

    if (shm_unlink(POSIX_SHOP) == -1)
        log("Failed to remove shared memory\n")
    else
        log("Removed shared memory\n");

    if (sem_close(shop_sem) == -1)
        log("Failed to close sem\n");
    if (sem_unlink(POSIX_SHOP_SEM) == -1)
        log("Failed to delete sem\n");

    if (sem_close(receiver_sem) == -1)
        log("Failed to close sem\n");
    if (sem_unlink(POSIX_RECEIVER_SEM) == -1)
        log("Failed to delete sem\n");

    if (sem_close(packer_sem) == -1)
        log("Failed to close sem\n");
    if (sem_unlink(POSIX_PACKER_SEM) == -1)
        log("Failed to delete sem\n");

    if (sem_close(sender_sem) == -1)
        log("Failed to close sem\n");
    if (sem_unlink(POSIX_SENDER_SEM) == -1)
        log("Failed to delete sem\n");
    log("Deleted all semaphores\n");
}

void int_handler(int signo) {
    exit(1);
}

int main() {

    if (atexit(clean) == -1)
        FAILURE_EXIT("Failed to setup cleaning handler");
    if (signal(SIGINT, int_handler) == SIG_ERR)
        FAILURE_EXIT("Failed to set sigint handler");

    create_fifo();
    create_semaphores();

    int receivers = 5;
    int packers = 5;
    int senders = 5;

    children_num = receivers + packers + senders;
    children = calloc(children_num, sizeof(int));

    for (int i = 0; i < receivers; i++) {
        int child = fork();
        if (child == 0) {
            execl("./worker", "./worker", "receiver", NULL);
            exit(0);
        }
        children[i] = child;
    }

    for (int i = 0; i < packers; i++) {
        int child = fork();
        if (child == 0) {
            execl("./worker", "./worker", "packer", NULL);
            exit(0);
        }
        children[receivers + i] = child;
    }

    for (int i = 0; i < senders; i++) {
        int child = fork();
        if (child == 0) {
            execl("./worker", "./worker", "sender", NULL);
            exit(0);
        }
        children[receivers + packers + i] = child;
    }
    for (int i = 0; i < children_num; i++) {
        waitpid(children[i], NULL, 0);
    }
}
