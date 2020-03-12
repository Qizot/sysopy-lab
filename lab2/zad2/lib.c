#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <ftw.h>
#include "lib.h"

struct time_point {
    time_t time;
    int more_than;
};

struct time_point atime, mtime;
int max_depth;

int compare_time(time_t first, time_t second, int more_than) {
    return more_than > 0 ? first > second : first < second;
}

void set_atime(int hours) {
    time(&atime.time);
    int sign = hours > 0 ? 1 : -1;
    atime.time -= sign * hours * 3600;
    if (hours >= 0) {
        atime.more_than = 0;
    } else {
        atime.more_than = 1;
    }
}

void set_mtime(int hours) {
    time(&mtime.time);
    int sign = hours > 0 ? 1 : -1;
    mtime.time -= sign * hours * 3600;
    if (hours >= 0) {
        mtime.more_than = 0;
    } else {
        mtime.more_than = 1;
    }
}

void set_max_depth(int depth) {
    max_depth = depth;
}

char* get_file_type(int st_mode){
    if(S_ISDIR(st_mode) != 0) return "dir";
    if(S_ISCHR(st_mode) != 0) return "char dev";
    if(S_ISBLK(st_mode) != 0) return "block dev";
    if(S_ISFIFO(st_mode) != 0) return "fifo";
    if(S_ISLNK(st_mode) != 0) return "slink";
    if(S_ISSOCK(st_mode) != 0) return "sock";
    if(S_ISREG(st_mode) != 0) return "file";
    return "error";
}

int get_date(time_t time, char *buffer){
    struct tm *times = localtime(&time);
    strftime(buffer, 256*sizeof(char), "%c", times);
    return 0;
}
int get_full_path(char *filename, char *buffer){
    if(realpath(filename, buffer) == NULL) return -1;
    return 0;
}



void traverse_directory(char *dirpath, int depth){
    if (depth <= 0) return;

    DIR* dir = opendir(dirpath);
    struct dirent *dirptr = NULL;
    struct stat *buffer = malloc(sizeof(struct stat));
    int display = 1;
    char modify_date[256];
    char access_date[256];
    char path[256];
    if(dir == NULL) return;
    chdir(dirpath);
    errno = 0;
    dirptr = readdir(dir);

    while(dirptr != NULL)
    {
        if(strcmp(dirptr->d_name, ".") == 0 || strcmp(dirptr->d_name,"..") == 0){
            dirptr = readdir(dir);
            continue;
        }
        if(lstat(dirptr->d_name, buffer) != 0) {
            system("pwd");
            printf("%s\n", strerror(errno));
            goto next_path; // encountered symbolic link
        }


        display = 1;
        if(!compare_time(buffer->st_mtime, mtime.time, mtime.more_than)) {
          display = 0;
        }
        if (!compare_time(buffer->st_atime, atime.time, atime.more_than)) {
            display = 0;
        }

        get_date(buffer->st_mtime, modify_date);
        get_date(buffer->st_atime, access_date);
        get_full_path(dirptr->d_name, path);
        if(display){
            printf("%s\t%d\t%s\t%d\t%s\t%s\n",
                   path,
                   (int)buffer->st_nlink,
                   get_file_type(buffer->st_mode),
                   (int)buffer->st_size,
                   access_date,
                   modify_date
            );
        }
        if(S_ISDIR(buffer->st_mode) != 0){
            traverse_directory(path, depth - 1);
        }

        next_path:

        chdir(dirpath);
        dirptr = readdir(dir);
    }
    free(buffer);
    closedir(dir);
}

void traverse(char* path) {
    traverse_directory(path, max_depth);
}

static int nftw_function(const char* fpath, const struct stat *file_stat, int typeflag, struct FTW* ftwbuf){
    if(ftwbuf->level > max_depth) return 0;
    char modify_date[256];
    char access_date[256];
    int display = 1;

    display = 1;
    if(!compare_time(file_stat->st_mtime, mtime.time, mtime.more_than)) {
        display = 0;
    }
    if (!compare_time(file_stat->st_atime, atime.time, atime.more_than)) {
        display = 0;
    }

    get_date(file_stat->st_mtime, modify_date);
    get_date(file_stat->st_atime, access_date);
    if(display){
        printf("%s\t%d\t%s\t%d\t%s\t%s\n",
               fpath,
               (int)file_stat->st_nlink,
               get_file_type(file_stat->st_mode),
               (int) file_stat->st_size,
               access_date,
               modify_date
        );
    }
    return 0;
}

void nftw_wrapper(char *path){
    nftw(path, nftw_function, 10, FTW_PHYS);
}