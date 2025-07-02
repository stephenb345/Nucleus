// write.c - POSIX 'write' system call
// Created 2024/04/07 by Stephen Byrne

#include "read.h"

extern bool tmpfs;
extern char *tmpfs_device_file;

ssize_t write(int filedes, const void *buf, size_t nbyte) {
  if (!descriptor_exists(filedes))
    return 1;
  if (tmpfs &&
      !strcmp(get_descriptor(filedes)->filename, (char *)"/dev/tmpfs")) {
    __uint128_t i, j;
    for (i = get_descriptor(filedes)->offset, j = 0;
         i < get_descriptor(filedes)->offset + nbyte; i++, j++) {
      
      map_page((uint64_t)(tmpfs_device_file + i));
      map_page((uint64_t)(((char *)buf)[j]));
      
      *(volatile char*)(tmpfs_device_file + i) = ((char *)buf)[j];
    }
  }
  return 0;
}