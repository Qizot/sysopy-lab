#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/times.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ftw.h>

static int nftw_function(const char* fpath, const struct stat *file_stat, int typeflag, struct FTW* ftwbuf){
    if(ftwbuf->level == 0) return 0;
    pid_t child_pid;
    if (typeflag == FTW_D) {
      child_pid = vfork();
      if (child_pid  == 0) {
          printf("\n\n%s\n", fpath);
          printf("PID: %d\n", getpid()) ;
          execlp("ls", "ls", "-l", fpath, NULL);
      } else {
          wait(NULL);
      }
    }
    return 0;
}

int main(int argc, char **argv){
    char* path = argv[1];
    char real_path[PATH_MAX + 1];
    realpath(path, real_path);
    nftw(real_path, nftw_function, 10, FTW_PHYS);
    return 0;
}