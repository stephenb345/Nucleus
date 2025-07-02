// handler_syscall.h - System call handler
// Created 2023/11/18 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "../utils/cstd/str.h"
#include "../proc/process.h"

#include "../posix/close.h"
#include "../posix/open.h"

#include "../posix/read.h"
#include "../posix/write.h"

#include "../posix/stat.h"

extern void send_syscall_return_val(void *);
void handle_syscall(int rax, void *arg0, void *arg1, void *arg2, void *arg3,
                   void *arg4, void *arg5);

#ifdef __cplusplus
}
#endif