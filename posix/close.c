// close.c - POSIX 'close' system call
// Created 2024/04/07 by Stephen Byrne

#include "close.h"

int close(int filedes) {
  if (!descriptor_exists(filedes))
    return 1;

  close_descriptor(filedes);
  return 0;
}