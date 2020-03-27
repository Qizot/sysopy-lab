#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <memory.h>
#include <sys/types.h>
#include <setjmp.h>

int current_signal;
sigjmp_buf point;

void signal_handler(int signo, siginfo_t* info, void* context) {

    printf("Received %s\n", strsignal(signo));
    printf("Current errno message: %s\n", strerror(info->si_errno));
    printf("Signal has been sent by user with id %d\n", info->si_uid);
    printf("Number of attempted system call %d\n", info->si_syscall);

    switch(info->si_code) {
        case SI_USER:
            puts("Signal has been sent by kill from other process");
            break;
        case SI_QUEUE:
            puts("Signal has been sent from sigqueue()");
            break;
        case SEGV_ACCERR:
            puts("Signal indicates that program had invalid permissions for mapped object");
            break;
        case CLD_EXITED:
            puts("Signal indicates child has exited");
            break;
        default:
            if (info->si_code <= 0) {
                puts("Signal has been sent by the same process");
                break;
            }
            puts("Unknown signal code");
            break;
    }
    // we arranged segfault so skip the corrupted program part
    if (info->si_signo == SIGSEGV) {
        longjmp(point, 1);
    }
}

void set_current_signal(int signo) {
    current_signal = signo;
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = signal_handler;

    if (sigaction(current_signal, &action, NULL) != 0) {
        perror("couldn't set sigaction\n");
        exit(EXIT_FAILURE);
    }
}



void send_signal_from_child() {
    set_current_signal(SIGUSR1);
    pid_t me = getpid();
    pid_t child = fork();
    if (child == 0) {
        kill(me, current_signal);
        exit(-1);
    }
    wait(NULL);
}

void generate_segfault() {
    set_current_signal(SIGSEGV);

    if (setjmp(point) == 0) {
        char* raw;
        raw[2] = 'A';
    } else {
        puts("Getting out of segfault...");
    }
}

void get_child_exit_signal() {
    set_current_signal(SIGCHLD);
    pid_t child = fork();
    if (child == 0) {
        exit(0);
    }
    wait(NULL);
}


int main() {

    system("sleep 1");
    get_child_exit_signal();
    return 0;
}
