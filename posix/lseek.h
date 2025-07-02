// lseek.h - POSIX 'lseek' system call
// Created 2024/08/27 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "../drivers/vfs/vfs.h"
#include "../proc/unix/file_descriptor.h"

typedef int32_t mode_t;
int lseek(int, __int128_t, mode_t);

#ifdef __cplusplus
}
#endif