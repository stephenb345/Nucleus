// allocator.c - Nucleus Memory Allocator
// Created 2024/04/06 by Stephen Byrne

#include "allocator.h"

size_t bytes_in_page = PAGE_SIZE;
size_t n_allocated_mem_tags = 0;

allocated_mem_tag *allocated_mem_tags = NULL;

extern void *memcpy(void *dest, const void *src, size_t n);
extern int memcmp(const void *s1, const void *s2, size_t n);
extern void *memset(void *s, int c, size_t n);

extern void println_int(long int n);

extern void map_page(uint64_t i);
extern bool using_custom_pages;

void *malloc(size_t bytes_to_alloc) {
  void *ret;
  int n_allocated_mem_tag = n_allocated_mem_tags;

  for (int i = 0; i < n_allocated_mem_tags; i++) {
    if (allocated_mem_tags[i].mem_ptr == NULL) {
      n_allocated_mem_tag = i;
      break;
    }
  }

  bytes_in_page += bytes_to_alloc;

  allocated_mem_tag tag;
  if (bytes_in_page >= PAGE_SIZE) {
    ret = allocator_alloc((bytes_to_alloc + PAGE_SIZE) / PAGE_SIZE);
    tag.containing_page = (unsigned int)(uintptr_t)ret;

    bytes_in_page = bytes_in_page % PAGE_SIZE;
    ret += bytes_in_page;
  } else {
    tag.containing_page = (unsigned int)(uintptr_t)(n_allocated_mem_tag + 1);
    ret = (void *)(uintptr_t)(((n_allocated_mem_tag + 1) * PAGE_SIZE) +
                              bytes_in_page);
  }

  tag.total_size = bytes_to_alloc;
  tag.mem_ptr = ret;
  tag.pages_occupied = ((bytes_to_alloc + PAGE_SIZE) / PAGE_SIZE) - 1;

  allocated_mem_tags[n_allocated_mem_tag] = tag;
  n_allocated_mem_tags++;

  if (using_custom_pages)
      map_page((uint64_t)ret);

  return ret;
}

void *realloc(void *prev_mem, size_t bytes_to_alloc) {
  void *ptr;
  int size = 0;
  int index = -1;

  if (bytes_to_alloc == 0) {
    free(prev_mem);
    return NULL;
  }

  if (prev_mem == NULL)
    return malloc(bytes_to_alloc);

  allocator_lock();

  for (unsigned int i = 0; i < n_allocated_mem_tags; i++) {
    if (prev_mem == allocated_mem_tags[i].mem_ptr) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    allocator_unlock();
    return NULL;
  }

  size = allocated_mem_tags[index].total_size;

  allocator_unlock();

  if (size > bytes_to_alloc)
    size = bytes_to_alloc;

  ptr = malloc(size);
  memcpy(ptr, prev_mem, allocated_mem_tags[index].total_size);

  free(prev_mem);

  return ptr;
}

void *calloc(size_t num, size_t bytes_to_alloc) {
  int size = num * bytes_to_alloc;
  void *ret = malloc(size);

  if (ret != NULL) {
    memset(ret, 0, size);
  }

  return ret;
}

void free(void *to_free) {
  if (to_free == NULL)
    return;
  int index = -1;

  allocator_lock();

  for (unsigned int i = 0; i < n_allocated_mem_tags; i++) {
    size_t remaining_size =
        allocated_mem_tags[i].total_size -
        ((size_t)(allocated_mem_tags[i].mem_ptr) - (size_t)(to_free));
    if (remaining_size >= allocated_mem_tags[i].total_size)
      continue;
    if (remaining_size >= allocated_mem_tags[i].total_size) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    allocator_unlock();
    return;
  }

  int num_bytes = allocated_mem_tags[index].total_size;
  for (unsigned int j = 0; j < n_allocated_mem_tags; j++) {
    if (allocated_mem_tags[index].containing_page ==
        allocated_mem_tags[j].containing_page) {
      num_bytes += allocated_mem_tags[j].total_size;
    }
  }

  if (num_bytes >= PAGE_SIZE) {
    allocator_free((void *)(intptr_t)allocated_mem_tags[index].containing_page,
                   (((int)num_bytes + PAGE_SIZE) / PAGE_SIZE) - 1);
  }

  n_allocated_mem_tags--;
  if (n_allocated_mem_tags < 0)
    n_allocated_mem_tags = 0;
  allocated_mem_tags[index].mem_ptr = NULL;

  allocator_unlock();
}
