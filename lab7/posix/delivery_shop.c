//
// Created by jakub on 21.04.2020.
//

#include "delivery_shop.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "utils.h"


int is_queue_full(delivery_shop_t* shop) {
    return shop->head == shop->tail ? 1 : 0;
}

int is_queue_empty(delivery_shop_t* shop) {
    return shop->head == -1 ? 1 : 0;
}

int forward_head(delivery_shop_t* shop) {
    // don't forward non-full queue
    if (is_queue_full(shop) == 0) {
        return -1;
    }
    int idx = (shop->head + 1) % MAX_ORDERS;
    if (shop->orders[idx].status == SENT || shop->orders[idx].status == EMPTY) {
        shop->orders[idx].status = EMPTY;
        shop->head = idx;
        return 0;
    }
    // couldn't forward head as leading order has not been prepared yet
    return -1;
}

void init_delivery_shop(delivery_shop_t* shop) {
    shop->created = 0;
    shop->prepared = 0;
    shop->head = -1;
    shop->tail = 0;
}

int push_order(delivery_shop_t *shop, order_t order) {
    if (is_queue_full(shop) == 1) {
        // couldn't forward head
        if (forward_head(shop) == -1) {
            return -1;
        }
    }
    // first item in queue
    if (shop->head == -1) {
        shop->head = 0;
    }

    order.status = CREATED;
    int idx = shop->tail;
    shop->orders[idx] = order;
    shop->tail = (idx + 1) % MAX_ORDERS;
    shop->created++;
    return idx;
}

int get_order(delivery_shop_t *shop, order_t *order, order_status status) {
    if (is_queue_empty(shop) == 1) {
        return -1;
    }
    // there is not option for getting SENT status
    if (status == SENT) {
        return -1;
    }

    int idx = shop->head;
    do {
        if (shop->orders[idx].status == status) {
            *order = shop->orders[idx];
            return idx;
        }
        idx = (idx + 1) % MAX_ORDERS;
    } while (idx != shop->tail);

    return -1;
}

int prepare_order(delivery_shop_t *shop, int idx) {
    if (idx < 0 || idx >= MAX_ORDERS) {
        return -1;
    }

    if (shop->orders[idx].status != CREATED) {
        return -1;
    }
    shop->orders[idx].status = PREPARED;
    shop->created--;
    shop->prepared++;
    return 0;
}

int send_order(delivery_shop_t *shop, int idx) {
    if (idx < 0 || idx >= MAX_ORDERS) {
        return -1;
    }
    if (shop->orders[idx].status != PREPARED) {
        return -1;
    }
    shop->orders[idx].status = SENT;
    shop->prepared--;
    return 0;
}

key_t get_shop_key() {
    char *path = getenv("HOME");
    if (path == NULL)
        FAILURE_EXIT("Failed to fetch HOME env");

    key_t key = ftok(path, SHOP_ID);
    if (key == -1)
        FAILURE_EXIT("Failed to create systemv key");
    return key;
}

char* get_current_time() {
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    int milli = curTime.tv_usec / 1000;

    time_t rawtime;
    struct tm* timeinfo;
    char buffer [80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);

    char* currentTime = calloc(84, sizeof(char));
    strcpy(currentTime, "");
    sprintf(currentTime, "%s:%d", buffer, milli);
    return currentTime;
}
