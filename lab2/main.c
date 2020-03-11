#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/times.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "lib.h"
#include <getopt.h>

#define READDIR 0
#define NFTW 1

int get_type(char* name) {
    if (strcmp(name, "nftw") == 0) {
        return NFTW;
    }
    if (strcmp(name, "readdir") == 0) {
        return READDIR;
    }
    return -1;
}

int main(int argc, char **argv){
    char *path = ".";
    int atime = 0;
    int mtime = 0;
    int type = -1;
    int max_depth = 1000;

    static struct option long_options[] = {
            {"type", required_argument, 0, 't'},
            {"mtime",   optional_argument, 0,  'm' },
            {"atime",   optional_argument, 0,  'a' },
            {"maxdepth",   required_argument, 0,  'd' },
            {"path",   required_argument, 0,  'p' },
            {0,           0,                 0,  0   }
    };

    int option;
    int long_index;
    while((option = getopt_long(argc, argv,"t:m:a:d:p:", long_options, &long_index )) != -1){ //get option from the getopt() method
        switch(option){
            case 't': {
                type = get_type(optarg);
                if (type == -1) {
                    fprintf(stderr, "invalid type argument, should be either  'nftw' or 'readdir'\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 'm': {
                mtime = atoi(optarg);
                break;
            }
            case 'a': {
                atime = atoi(optarg);
                break;
            }
            case 'd': {
              max_depth = atoi(optarg);
              break;
            }
            case 'p': {
                path = optarg;
                break;
            }
            default: break;
        }
    }
    set_atime(atime);
    set_mtime(mtime);
    set_max_depth(max_depth);

    if (type == NFTW) {
      nftw_wrapper(path);
    } else if (type == READDIR) {
        traverse(path);
     }


    return 0;
}