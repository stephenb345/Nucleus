// read.c - POSIX 'read' system call
// Created 2024/04/07 by Stephen Byrne

#include "read.h"

extern bool tmpfs;
extern char *tmpfs_device_file;

ssize_t read(int fd, void *buf, size_t count) {
  if (!descriptor_exists(fd))
    return 1;

  __uint128_t i, j;
  for (i = get_descriptor(fd)->offset, j = 0;
       i < get_descriptor(fd)->offset + count; i++, j++) {
    ((char *)buf)[j] = (char)*(char*)(tmpfs_device_file + i);
  }
  return 0;
}