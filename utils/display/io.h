// io.h - Terminal and input/output utilituies
// Created 2023/7/11 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "../../lib/flanterm/backends/fb.h"
#include "../../lib/flanterm/flanterm.h"

#include "../../include/global.h"
#include "../cstd/str.h"
#include "../cstd/mem.h"

extern NUCLEUS_PARAMETERS *boot_parameters;

void syslog(const char *str);

void println(const char *str);
void print(const char *str);

void println_int(__int128_t n);
void print_int(__int128_t n);

void clear_term();

void kernel_panic();

#ifdef __cplusplus
}
#endif