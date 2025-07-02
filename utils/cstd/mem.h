// mem.h - C standard library compliant memory function replacements
// Created 2023/7/10 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_SIZE 0x1000

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../display/io.h"
#include "memalloc.h"
#include "../../mem/paging/paging.h"

extern bool is_locked;

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

void mem_lock_acquire(atomic_flag *lock);
void mem_lock_release(atomic_flag *lock);

#ifdef __cplusplus
}
#endif