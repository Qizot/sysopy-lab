#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define IGNORE 1
#define HANDLER 2
#define MASK 3
#define PENDING 4

int get_mode(char* mode) {
    if (strcmp(mode, "ignore") == 0) {
        return IGNORE;
    }
    if (strcmp(mode, "handler") == 0) {
        return HANDLER;
    }
    if (strcmp(mode, "mask") == 0) {
        return MASK;
    }
    if (strcmp(mode, "pending") == 0) {
        return PENDING;
    }
    return -1;
}

int main(int argc, char** argv) {
    printf("Entered child process\n");
    if (argc != 2) {
        printf("child program expected single mode argument (ignore, handler, mask, pending)");
        exit(EXIT_FAILURE);
    }
    int mode = get_mode(argv[1]);
    if (mode == -1) {
        printf("Invalid mode argument: should be one of (ignore, handler, mask, pending)\n");
        exit(EXIT_FAILURE);
    }

    sigset_t child_pending_signals;
    if (mode != PENDING) {
        raise(SIGUSR1);
    }
    if (sigpending(&child_pending_signals) == 0) {
        if (sigismember(&child_pending_signals, SIGUSR1) == 1) {
            printf("Child process: signal is pending\n");
        } else {
            printf("Child process: signal is NOT pending\n");
        }
    } else {
        printf("Child: Failed to get pending signals\n");
    }

    return 0;
}
