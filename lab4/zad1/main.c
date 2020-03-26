#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int halt = 0;

void toggle_process_halt(int sig_num) {
    if(halt == 0) {
        printf("\nOczekuję na CTRL+Z - kontynuacja albo CTR+C - zakończenie programu\n");
    }
    halt = halt == 1 ? 0 : 1;
}

void init_signal() {
    printf("\nOdebrano sygnał SIGINT\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {

    struct sigaction actions;
    actions.sa_handler = toggle_process_halt;
    actions.sa_flags = 0;
    sigemptyset(&actions.sa_mask);


    while(1) {
        sigaction(SIGTSTP, &actions, NULL);
        signal(SIGINT, init_signal);

        if (!halt) {
            system("ls");
        }
        sleep(1);
    }
}