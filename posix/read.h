// read.h - POSIX 'read' system call
// Created 2024/04/07 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "../drivers/vfs/vfs.h"
#include "../proc/unix/file_descriptor.h"

typedef size_t ssize_t;
ssize_t read(int fd, void *buf, size_t count);

#ifdef __cplusplus
}
#endif