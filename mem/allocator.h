// allocator.h - Nucleus Memory Allocator
// Created 2024/04/06 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include "../include/global.h"

#define PAGE_SIZE 0x1000

typedef struct {
  unsigned int pages_occupied;
  size_t total_size;
  unsigned int containing_page;
  void *mem_ptr;
} allocated_mem_tag;

extern void allocator_lock();
extern void allocator_unlock();
extern void *allocator_alloc(size_t);
extern int allocator_free(void *, size_t);

void *malloc(size_t);
void *realloc(void *, size_t);
void *calloc(size_t, size_t);
void free(void *);

#ifdef __cplusplus
}
#endif