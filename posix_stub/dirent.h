/*
    PicOS dirent.h stub — newlib-arm doesn't provide one.
    Minimal implementation for tinydir and C-Dogs.
*/
#pragma once

#include <stddef.h>

struct dirent {
    char d_name[256];
};

typedef struct {
    void *__opaque;
} DIR;

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

#include <sys/stat.h>
int lstat(const char *path, struct stat *buf);
