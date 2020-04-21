//
// Created by jakub on 21.04.2020.
//

#ifndef SYSTEMV_DELIVERY_SHOP_H
#define SYSTEMV_DELIVERY_SHOP_H

#include <sys/ipc.h>

#define MAX_ORDERS 1024
#define SHOP_ID 66
#define POSIX_SHOP "shop_memory"
#define POSIX_SHOP_SEM "shop_sem"
#define POSIX_RECEIVER_SEM "receiver_sem"
#define POSIX_SENDER_SEM "sender_sem"
#define POSIX_PACKER_SEM "packer_sem"

// created means ready for preparation
// prepared means ready for delivery
// sent means that order can be replaced in delivery_shop queue
typedef enum order_status {
    EMPTY = 0,
    CREATED = 1,
    PREPARED = 2,
    SENT = 3
} order_status;

typedef struct order {
    order_status status;
    int n;
} order_t;


// receiver creates new order
// packer packs order and makes it ready for sending
// sender sends order
typedef enum worker_sem {
    SHOP = 0,
    RECEIVER = 1,
    PACKER = 2,
    SENDER = 3
} worker_sem;

typedef struct delivery_shop {
    int created;
    int prepared;
    int head;
    int tail;
    order_t orders[MAX_ORDERS];
} delivery_shop_t;


// returns current time string with miliseconds included
// must be freed afterwards
char* get_current_time();

key_t get_shop_key();

int is_queue_full(delivery_shop_t* shop);

int is_queue_empty(delivery_shop_t* shop);

// initialize new delivery shop
void init_delivery_shop(delivery_shop_t* shop);

// pushes order to shop queue with status CREATED
// if queue is full returns -1
int push_order(delivery_shop_t* shop, order_t order);

// gets first order that matches given status and assigns it to order pointer, returns order idx in queue
// if queue is empty or message with requested status doesn't exist returns -1
int get_order(delivery_shop_t* shop, order_t* order, order_status status);

// changes order status to prepared and updates counter in delivery_shop, increments prepared, decrements created
int prepare_order(delivery_shop_t* shop, int idx);

// changes order status to sent and decrements prepared
int send_order(delivery_shop_t* shop, int idx);


#endif //SYSTEMV_DELIVERY_SHOP_H
