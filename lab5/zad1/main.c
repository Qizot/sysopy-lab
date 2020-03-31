#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <libgen.h>
#include <time.h>
#include <errno.h>
#include <signal.h>


#define MAX_LINE_LEN 1024
#define MAX_COMMANDS 256
#define MAX_ARGUMENTS 64


char** parse_command(char* command) {
    char delimiters[3] = {' ', '\n', '\0'};
    char** parsed = calloc(MAX_ARGUMENTS, sizeof(char*));
    int i = 0;
    while (i < MAX_ARGUMENTS && (parsed[i] = strtok(i == 0 ? command : NULL, delimiters)) != NULL) {
        i++;
    }

    parsed = realloc(parsed, sizeof(char*) * (i + 1));

    if (parsed == NULL) {
        perror("failed to realloc");
        exit(EXIT_FAILURE);
    }

    parsed[i] = NULL;
    return parsed;
}


char** parse_line_to_commands(char* line, int* size) {
    char delimiters[2] = {'|', '\0'};
    char** parsed = calloc(MAX_COMMANDS, sizeof(char*));
    int i = 0;
    while (i < MAX_COMMANDS && (parsed[i] = strtok(i == 0 ? line : NULL, delimiters)) != NULL) {
      i++;
    }

    parsed = realloc(parsed, sizeof(char*) * (i + 1));
    if (parsed == NULL) {
        perror("failed to realloc");
        exit(EXIT_FAILURE);
    }

    parsed[i] = NULL;
    *size = i;
    return parsed;
}


void execute_line(char* line) {
    int commands_number = 0;
    char** commands = parse_line_to_commands(line, &commands_number);
    int pipes[2][2];
    int i;
    for (i = 0; i < commands_number; i++) {
    //
    // close file descriptors of previous process
        if (i > 0) {
            close(pipes[i % 2][0]);
            close(pipes[i % 2][1]);
        }

        if (pipe(pipes[i % 2]) == -1) {
            perror("failed to open pipe");
            exit(EXIT_FAILURE);
        }


        pid_t child = fork();
        if (child == 0) {
            char** command = parse_command(commands[i]);

            // pipe only if we have another commands to pipe to
            if (i != commands_number - 1) {
                if (dup2(pipes[i % 2][1], STDOUT_FILENO) < 0) {
                    perror("failed to dup2");
                    exit(EXIT_FAILURE);
                }
            }

            if (i >  0) {
                close(pipes[(i + 1) % 2][1]);
                if (dup2(pipes[(i + 1) % 2][0], STDIN_FILENO) < 0) {
                    perror("failed to dup2");
                    exit(EXIT_FAILURE);
                }
            }
            execvp(command[0], command);
            exit(0);
        }
    }
    close(pipes[i % 2][0]);
    close(pipes[i % 2][1]);
    wait(NULL);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    char buffer[MAX_LINE_LEN];

    if(argc != 2) {
        puts("program has expected single argument <filename>");
        exit(EXIT_FAILURE);
    }

    FILE* file = fopen(argv[1],"r");
    if(file == NULL) {
        puts("failed to open file");
        exit(EXIT_FAILURE);
    }


    int executed = 0;
    while(fgets(buffer, sizeof(buffer), file)) {
        executed++;
        pid_t child = fork();
        if (child == 0) {
            fclose(file);
            execute_line(buffer);
            exit(EXIT_SUCCESS);
        }
        int status;
        wait(&status);
        if (status) {
          printf("Failed to execute line: %d\n", executed);
          exit(EXIT_FAILURE);
        }
        memset(buffer, 0, sizeof(buffer));
    }
    fclose(file);
    return 0;
}
