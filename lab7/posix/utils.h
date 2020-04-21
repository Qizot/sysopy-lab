//
// Created by jakub on 21.04.2020.
//

#ifndef SYSTEMV_UTILS_H
#define SYSTEMV_UTILS_H

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#define log(format, ...) {printf("\033[1;31m %s:%d %d: \033[0m [errno: %s] ", __FILE__, __LINE__, getpid(), strerror(errno)); printf(format, ##__VA_ARGS__);}
#define FAILURE_EXIT(format, ...) { log(format, ##__VA_ARGS__); exit(EXIT_FAILURE);}

#endif //SYSTEMV_UTILS_H
