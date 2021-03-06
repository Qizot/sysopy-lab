//
// Created by jakub on 26.03.2020.
//

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <memory.h>
#include <sys/types.h>

#define IGNORE 1
#define HANDLER 2
#define MASK 3
#define PENDING 4

#define FORK 1
#define EXEC 2

#define EXIT_WITH_MESSAGE(format, ...) { fprintf(stderr, format, ##__VA_ARGS__); exit(EXIT_FAILURE);}
#define ASSERT_ARGUMENT(argument, condition) if (condition) { printf("Invalid %s value", #argument); exit(EXIT_FAILURE);}

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

char* get_mode_string(int mode) {
    switch(mode) {
        case IGNORE:
            return "ignore";
        case HANDLER:
            return "handler";
        case MASK:
            return "mask";
        case PENDING:
            return "pending";
        default:
            return "unknown";
    }
}

int get_type(char* type) {
    if (strcmp(type, "fork") == 0) {
        return FORK;
    }
    if (strcmp(type, "exec") == 0) {
        return EXEC;
    }
    return -1;
}

void usr_handler(int sig, siginfo_t* info, void* context) {
    printf("Received SIGUSR1 in process %d from process %d\n", getpid(), info->si_pid);
}

int main(int argc, char *argv[]) {
    ASSERT_ARGUMENT(argc,(argc != 3));
    int mode = get_mode(argv[1]);
    int type = get_type(argv[2]);
    if (mode == -1) {
        printf("Invalid mode argument: should be one of (ignore, handler, mask, pending)\n");
        exit(EXIT_FAILURE);
    }
    if (type == -1) {
        printf("Invalid type argument: should be one of (fork, exec)\n");
        exit(EXIT_FAILURE);
    }

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;

    if (mode == IGNORE) {
        act.sa_sigaction = SIG_IGN;
    }
    if (mode == HANDLER) {
        act.sa_sigaction = usr_handler;
    }
    if (mode == MASK || mode == PENDING) {
        sigaddset(&act.sa_mask, SIGUSR1);
        if (sigprocmask(SIG_BLOCK, &act.sa_mask,NULL) != 0) {
            EXIT_WITH_MESSAGE("Failed to set proc mask\n");
        };
    }
    if (sigaction(SIGUSR1, &act, NULL) == -1) EXIT_WITH_MESSAGE("Failed to set sigaction\n");

    raise(SIGUSR1);
    sigset_t pending_signals;
    if (sigpending(&pending_signals) == 0) {
        if (sigismember(&pending_signals, SIGUSR1) == 1) {
            printf("Parent process: signal is pending\n");
        } else {
            printf("Parent process: signal is NOT pending\n");
        }
    } else {
        printf("Parent: Failed to get pending signals");
    }

    if (type == FORK) {
        pid_t child = fork();
        if (child == 0) {
            sigset_t child_pending_signals;
            if (sigpending(&child_pending_signals) == 0) {
                if (sigismember(&child_pending_signals, SIGUSR1) == 1) {
                    printf("Child process: signal is pending\n");
                } else {
                    printf("Child process: signal is NOT pending\n");
                }
            } else {
                printf("Child: Failed to get pending signals\n");
            }
            if (mode != PENDING) {
                raise(SIGUSR1);
            }
        }
    }
    if (type == EXEC) {
        char* string_mode = get_mode_string(mode);
        pid_t child = fork();
        if (child == 0) {
            execl("./child", "./child", string_mode, NULL);
            exit(EXIT_FAILURE);
        } else {
            wait(NULL);
        }
    }


}
