// open.h - POSIX 'open' system call
// Created 2024/04/07 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../proc/unix/file_descriptor.h"
#include <stdarg.h>
#include <stdint.h>

typedef int32_t mode_t;
int open(const char *path, int oflag, ...);

#ifdef __cplusplus
}
#endif