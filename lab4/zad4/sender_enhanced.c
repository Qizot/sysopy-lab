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
int signals;
int mode = -1;
int confirmed = 0;

int get_mode(char* modestr) {

    if (strcmp(modestr, "kill") == 0) {
        return KILL;
    }
    if (strcmp(modestr, "queue") == 0) {
        return QUEUE;
    }
    if (strcmp(modestr, "rt") == 0) {
        return RT;
    }
    return -1;
}

void send_signal(int pid, int i){
    union sigval value;
    switch(mode){
        case KILL:
            kill(pid, SIGUSR1);
            break;
        case QUEUE:
            value.sival_int = i;
            sigqueue(pid,SIGUSR1,value);
            break;
        case RT:
            kill(pid,SIGRTMIN);
            break;
    }
}

void send_termination_signal(int pid){
    union sigval value;
    switch(mode){
        case KILL:
            kill(pid, SIGUSR2);
            break;
        case QUEUE:
            value.sival_int = -1;
            sigqueue(pid,SIGUSR2,value);
            break;
        case RT:
            kill(pid, SIGRTMIN+1);
            break;
    }
}

void count_function(int sig,siginfo_t *info, void *context){
    count = count + 1;
    if(info->si_code == SI_QUEUE){
        printf("Received %d values from catcher\n", info->si_value.sival_int);
    }
    confirmed = 1;
}

void termination_function(int sig,siginfo_t *info, void *context){
    printf("Sent %d signals to catcher\n", signals);
    printf("Received %d signals from catcher\n", count);
    exit(0);
}

void set_count_handler(){
    struct sigaction action;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    switch(mode){
        case KILL:
            action.sa_sigaction = &count_function;
            if (sigaction(SIGUSR1,&action,NULL) != 0) {
                printf("error setting sigaction\n");
                exit(EXIT_FAILURE);
            }
            break;
        case QUEUE:
            action.sa_sigaction = &count_function;
            if (sigaction(SIGUSR1,&action,NULL) != 0) {
                printf("error setting sigaction\n");
                exit(EXIT_FAILURE);
            }
            break;
        case RT:
            action.sa_sigaction = &count_function;
            sigaddset(&action.sa_mask,SIGRTMIN+1);
            if (sigaction(SIGRTMIN,&action,NULL) != 0) {
                printf("error setting sigaction\n");
                exit(EXIT_FAILURE);
            }
            break;
    }
}

void set_termination_handler(){
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = &termination_function;
    switch(mode){
        case KILL:
            if (sigaction(SIGUSR2,&action,NULL) != 0) {
                printf("error setting sigaction\n");
                exit(EXIT_FAILURE);
            }
            break;
        case QUEUE:
            if (sigaction(SIGUSR2,&action,NULL) != 0) {
                printf("error setting sigaction\n");
                exit(EXIT_FAILURE);
            }
            break;
        case RT:
            if (sigaction(SIGRTMIN+1,&action,NULL) != 0) {
                printf("error setting sigaction\n");
                exit(EXIT_FAILURE);
            }
            break;
    }
}

int main(int argc, char **argv){
    int catcher_pid;
    if(argc != 4){
        printf("3 arguments expected: catcher's pid, number of signals, one of (kill, queue, ri)3 arguments expected: catcher's pid, number of signals, one of (kill, queue, ri)3 arguments expected: catcher's pid, number of signals, one of (kill, queue, ri)3 arguments expected: catcher's pid, number of signals, one of (kill, queue, ri)3 arguments expected: catcher's pid, number of signals, one of (kill, queue, ri)3 arguments expected: catcher's pid, number of signals, one of (kill, queue, ri)3 arguments expected: catcher's pid, number of signals, one of (kill, queue, ri)3 arguments expected: catcher's pid, number of signals, one of (kill, queue, ri)3 arguments expected: catcher's pid, number of signals, one of (kill, queue, ri)\n");
        return -1;
    }
    if(sscanf(argv[1],"%d",&catcher_pid) != 1){
        printf("invalid catcher's pid\n");
        return -1;
    }
    if(sscanf(argv[2],"%d",&signals) != 1){
        printf("invalid number of signals\n");
        return -1;
    }
    mode = get_mode(argv[3]);
    if(mode == -1){
        printf("invalid mode: one of the following arguments expected: kill, queue, rt\n");
        return -1;
    }

    sigset_t masked_set;
    sigset_t old_set;

    sigfillset(&masked_set);
    sigdelset(&masked_set,SIGUSR1);
    sigdelset(&masked_set,SIGUSR2);
    sigdelset(&masked_set,SIGRTMIN);
    sigdelset(&masked_set,SIGRTMIN+1);
    sigprocmask(SIG_SETMASK,&masked_set,&old_set);

    set_count_handler();
    set_termination_handler();

    for(int i = 0; i < signals; i++){
        confirmed = 0;
        send_signal(catcher_pid, i);
        while (confirmed == 0) {
            pause();
        }
    }
    send_termination_signal(catcher_pid);
    pause();
    return 0;
}


