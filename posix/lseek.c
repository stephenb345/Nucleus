// read.c - POSIX 'lseek' system call
// Created 2024/08/27 by Stephen Byrne

#include "lseek.h"

extern bool tmpfs;
extern uint64_t tmpfs_size;

extern __uint128_t fsize;

int lseek(int fd, __int128_t offset, mode_t mode) {
  if (!descriptor_exists(fd))
    return 1;
  if (!tmpfs && strcmp(get_descriptor(fd)->filename, (char*)"/dev/tmpfs"))
    vrdfile((char*)get_descriptor(fd)->filename);
  if (mode == 0)
    get_descriptor(fd)->offset = offset;
  else if (mode == 1)
    get_descriptor(fd)->offset += offset;
  else if (mode == 2)
    get_descriptor(fd)->offset =
        (tmpfs && !strcmp(get_descriptor(fd)->filename, (char*)"/dev/tmpfs")) ? ((tmpfs_size) - 1)
              : ((__int128_t)fsize + offset);
  return 0;
}