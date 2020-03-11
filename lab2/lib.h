#ifndef LIB_H
#define LIB_H

extern char op;
extern time_t time_from_epoch;

void traverse(char* path);

void nftw_wrapper(char *path);

void set_atime(int hours);

void set_mtime(int horus);

void set_max_depth(int depth);

#endif