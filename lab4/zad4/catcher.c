//
// Created by jakub on 26.03.2020.
//

#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libgen.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#define KILL 1
#define QUEUE 2
#define RT 3

int count = 0;
int sender_pid = -1;

int get_mode(char* mode) {

    if (strcmp(mode, "kill") == 0) {
        return KILL;
    }
    if (strcmp(mode, "queue") == 0) {
        return QUEUE;
    }
    if (strcmp(mode, "rt") == 0) {
        return RT;
    }
    return -1;
}

void count_function(int sig, siginfo_t *info, void *context) {
    count = count + 1;
    sender_pid = info->si_pid;
}

void on_kill_end(int sig, siginfo_t *info, void *context) {
    if (sender_pid != -1) {
        for (int i = 0; i < count; i++) {
            kill(sender_pid, SIGUSR1);
        }
        kill(sender_pid, SIGUSR2);
    }
    printf("Catcher: received %d signals \n",count);
    exit(0);
}

void on_queue_end(int sig, siginfo_t *info, void* context) {
    printf("received signal to kill %d\n", info->si_signo);
    union sigval value;
    if (sender_pid != -1) {
        for (int i = 0; i < count; i++) {
            value.sival_int = i;
            sigqueue(sender_pid, SIGUSR1, value);
        }
        value.sival_int = count;
        sigqueue(sender_pid, SIGUSR2, value);
    }
    printf("Catcher: received %d signals \n",count);
    exit(0);
}
void on_rt_end(int sig, siginfo_t *info, void* context) {
    if (sender_pid != -1) {
        for (int i = 0; i < count; i++) {
            kill(sender_pid, SIGRTMIN);
        }
        kill(sender_pid, SIGRTMIN+1);
    }
    printf("Catcher: received %d signals \n",count);
    exit(0);
}


void set_count_handler(int mode) {
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;

    switch(mode) {
        case KILL:
        case QUEUE:
            action.sa_sigaction = &count_function;
            if (sigaction(SIGUSR1,&action,NULL) != 0) {
                printf("got error while sigaction\n:");
                exit(-1);
            }
            break;
        case RT:
            action.sa_sigaction = &count_function;
            sigaddset(&action.sa_mask,SIGRTMIN+1);
            if (sigaction(SIGRTMIN,&action,NULL) != 0) {
                printf("got error while sigaction\n");
                exit(-1);
            }
            break;
    }
}

void set_end_handler(int mode) {
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;

    switch(mode) {
        case KILL:
            action.sa_sigaction = &on_kill_end;
            if (sigaction(SIGUSR2,&action,NULL) != 0) {
                printf("got error while sigaction\n");
                exit(-1);
            }
            break;
        case QUEUE:
            action.sa_sigaction = &on_queue_end;
            if (sigaction(SIGUSR2,&action,NULL) != 0) {
                printf("got error while sigaction\n");
                exit(-1);
            }
            break;
        case RT:
            action.sa_sigaction = &on_rt_end;
            if (sigaction(SIGRTMIN+1,&action,NULL) != 0) {
                printf("got error while sigaction\n");
                exit(-1);
            }
            break;
    }
}

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("one of the following arguments expected: kill, queue, rt\n");
        return -1;
    }
    int mode = get_mode(argv[1]);
    if(mode == -1) {
        printf("invalid mode\n");
        return -1;
    }

    set_count_handler(mode);
    set_end_handler(mode);

    sigset_t masked_set;
    sigset_t old_set;

    // block every signal except for defined below
    sigfillset(&masked_set);
    sigdelset(&masked_set,SIGUSR1);
    sigdelset(&masked_set,SIGUSR2);
    sigdelset(&masked_set,SIGRTMIN);
    sigdelset(&masked_set,SIGRTMIN+1);
    sigprocmask(SIG_SETMASK,&masked_set,&old_set);

    printf("PID: %d \n",getpid());
    while(1) {
        pause();
    }
    return 0;
}