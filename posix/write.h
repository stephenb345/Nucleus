// write.h - POSIX 'write' system call
// Created 2024/04/07 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "../drivers/vfs/vfs.h"
#include "../proc/unix/file_descriptor.h"

typedef size_t ssize_t;
ssize_t write(int fildes, const void *buf, size_t nbyte);

#ifdef __cplusplus
}
#endif