// memalloc.h - Advanced memory allocation utilities
// Created 2023/11/10 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../../mem/allocator.h"
#include "mem.h"

void init_mem(void);
void *alloc_page(size_t);
void *bootstrap_alloc_page(size_t, uint8_t *);
void free_page(void *, size_t);

#ifdef __cplusplus
}
#endif