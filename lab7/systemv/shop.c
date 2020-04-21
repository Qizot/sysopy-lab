//
// Created by jakub on 21.04.2020.
//

#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>
#include "delivery_shop.h"
#include "utils.h"

key_t fifo_key = -1;
int shm_id = -1;
int sem_id = -1;
delivery_shop_t* shop = NULL;
int children_num = 0;
int *children = NULL;

void create_fifo() {
   fifo_key = get_shop_key();

   shm_id = shmget(fifo_key, sizeof(delivery_shop_t), IPC_CREAT | IPC_EXCL | 0666);
   if (shm_id == -1)
       FAILURE_EXIT("Failed to shmget");

   void *tmp = shmat(shm_id, NULL, 0);
   if (tmp == (void*)(-1))
       FAILURE_EXIT("Failed to shmat");
   shop = (delivery_shop_t*) tmp;
   init_delivery_shop(shop);
}

void create_semaphores() {
    sem_id = semget(fifo_key, 4, IPC_CREAT | IPC_EXCL | 0666);
    if (sem_id == -1)
        FAILURE_EXIT("Failed to semget");

    for (int i = 1; i <= 3; i++) {
        if (semctl(sem_id, i, SETVAL, 1) == -1)
            FAILURE_EXIT("Failed to semctl 3 basic semaphores");
    }
    if (semctl(sem_id, SHOP, SETVAL, 1) == -1)
        FAILURE_EXIT("Failed to semctl for SHOP sempahore");

}


void clean() {
    log("Cleaning shop resources...\n");
    for (int i = 0; i < children_num; i++) {
        if (children[i] > 0) {
            kill(children[i], SIGINT);
        }
    }
    if (shmdt(shop) == -1)
       log("Failed to disconnect shared memory of shop\n")
    else
        log("Disconnected shared memory\n");

    if (shmctl(shm_id, IPC_RMID, NULL) == -1)
        log("Failed to remove shared memory\n")
    else
        log("Removed shared memory\n");

    if (semctl(sem_id, 0, IPC_RMID) == -1)
        log("Failed to remove semaphores\n")
    else
        log("Removed semaphores\n");
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
