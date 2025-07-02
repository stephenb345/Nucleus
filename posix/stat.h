// stat.h - POSIX 'stat' system call
// Created 2024/11/29 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../proc/unix/file_descriptor.h"
#include <stdarg.h>
#include <stdint.h>

typedef struct {
  unsigned short st_dev;
  unsigned short st_ino;
  unsigned short st_mode;
  unsigned short st_nlink;
  unsigned short st_uid;
  unsigned short st_gid;
  unsigned short st_rdev;
  unsigned long st_size;
  unsigned long st_atime;
  unsigned long st_ctime;
} s_stat;

int stat(const char*, s_stat *);
int lstat(const char *, s_stat *);

#ifdef __cplusplus
}
#endif