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

void usr_handler(int sig) {
    printf("Received SIGUSR1 signal\n");
}


void check_pending_signal(char* header, int signo) {
    sigset_t pending_signals;
    if (sigpending(&pending_signals) == 0) {
        if (sigismember(&pending_signals, signo) == 1) {
            printf("%s process: signal is pending\n", header);
        } else {
            printf("%s process: signal is NOT pending\n", header);
        }
    } else {
        printf("%s: Failed to get pending signals", header);
    }
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
    int current_signal = SIGUSR1;


    if (mode == IGNORE) {
        signal(current_signal, SIG_IGN);
    }
    if (mode == HANDLER) {
        signal(current_signal, usr_handler);
    }
    if (mode == MASK || mode == PENDING) {
        sigset_t new_mask, old_mask;
        sigemptyset(&new_mask);

        sigaddset(&new_mask, current_signal);
        if (sigprocmask(SIG_BLOCK, &new_mask,&old_mask) != 0) {
            EXIT_WITH_MESSAGE("Failed to set proc mask\n");
        };
    }

    printf("Before raise\n");
    check_pending_signal("Parent", current_signal);

    raise(current_signal);
    printf("After raise\n");
    check_pending_signal("Parent", current_signal);

    if (type == FORK) {
        pid_t child = fork();
        if (child == 0) {
            printf("Before raise\n");
            check_pending_signal("Child", current_signal);

            if (mode != PENDING) {
                printf("After raise\n");
                raise(current_signal);
                check_pending_signal("Child", current_signal);
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
