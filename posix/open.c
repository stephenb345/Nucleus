// open.c - POSIX 'open' system call
// Created 2024/04/07 by Stephen Byrne

#include "open.h"

int open(const char *path, int oflag, ...) {
  va_list args;
  va_start(args, oflag);

  int val = (int)(mk_descriptor((char *)path, oflag,
                                va_arg(args, char*))
                      ->descriptor_id);
  va_end(args);

  return val;
}