// memalloc.c - Advanced memory allocation utilities
// Created 2023/11/10 by Stephen Byrne

#include "memalloc.h"

extern NUCLEUS_PARAMETERS *boot_parameters;
uint8_t *mem_buf;

extern bool using_custom_pages;

int64_t malloc_phys_page(size_t num_pages) {
  size_t buf_size = boot_parameters->usable_mem_size / PAGE_SIZE;
  size_t max_index = buf_size - num_pages + 1;
  for (size_t i = 0; i < max_index; i++) {
    size_t j;
    for (j = 0; j < num_pages; j++) {
      if (mem_buf[i + j] != 0x00)
        break;
    }
    if (j == num_pages) {
      for (j = 0; j < num_pages; j++)
        mem_buf[i + j] = 0x01;
      return i * PAGE_SIZE;
    }
    i += j;
  }
  return -1;
}

void *alloc_page(size_t pages) {
  int64_t first_page_address = malloc_phys_page(pages);
  if (first_page_address == -1)
    return NULL;
  return (void *)(uint64_t)first_page_address;
}

int64_t bootstrap_malloc_phys_page(size_t num_pages,
                                  uint8_t *bootstrap_mem_buf) {
  size_t buf_size = boot_parameters->usable_mem_size / PAGE_SIZE;
  size_t max_index = buf_size - num_pages + 1;
  for (size_t i = 0; i < max_index; i++) {
    size_t j;
    for (j = 0; j < num_pages; j++) {
      if (bootstrap_mem_buf[i + j] != 0x00)
        break;
    }
    if (j == num_pages) {
      for (j = 0; j < num_pages; j++)
        bootstrap_mem_buf[i + j] = 0x01;
      return i * PAGE_SIZE;
    }
    i += j;
  }
  return -1;
}

void *bootstrap_alloc_page(size_t pages, uint8_t *bootstrap_mem_buf) {
  int64_t first_page_index = bootstrap_malloc_phys_page(pages, bootstrap_mem_buf);
  if (first_page_index == -1)
    return NULL;
  return (void *)(uint64_t)(first_page_index * PAGE_SIZE);
}

void free_page(void *position, size_t pages) {
  for (size_t i = (int)(uint64_t)position / PAGE_SIZE;
       i < ((int)(uint64_t)position / PAGE_SIZE) + pages; i++) {
    mem_buf[i] = 0x00;
  }
}

extern allocated_mem_tag *allocated_mem_tags;
extern int n_allocated_mem_tags;

void init_mem() {   
  SYSLOG(SteveStudiosNucleusBitmapAllocator, Starting...)

  size_t buf_size = boot_parameters->usable_mem_size / PAGE_SIZE;
  uint8_t tmp_mem_buf[buf_size];
  SYSLOG(SteveStudiosNucleusBitmapAllocator, Got free pages...)

  extern unsigned long kernel_start;

  for (size_t i = 0; i < buf_size; i++) {
    if (i * PAGE_SIZE <= PAGE_SIZE)
      tmp_mem_buf[i] = 0x01;
    else
      tmp_mem_buf[i] = 0x00;
  }

  SYSLOG(SteveStudiosNucleusBitmapAllocator, Marked all as unused...)
  SYSLOG(SteveStudiosNucleusBitmapAllocator, done)

  allocator_lock();

  SYSLOG(SteveStudiosNucleusBitmapAllocator, Allocate bootstrap page for memory bitmap...)

  mem_buf = (uint8_t *)bootstrap_alloc_page(buf_size / PAGE_SIZE,
                                            (uint8_t *)tmp_mem_buf);

  SYSLOG(SteveStudiosNucleusBitmapAllocator, done)

  allocator_unlock();

  SYSLOG(SteveStudiosNucleusBitmapAllocator,
         Copy temporary buffer to memory bitmap...)

  for (int i = 0; i < buf_size; i++) {
    mem_buf[i] = tmp_mem_buf[i];
  }

  SYSLOG(SteveStudiosNucleusBitmapAllocator,
         Copied temporary buffer to memory bitmap)

  allocator_lock();

  allocated_mem_tags = (allocated_mem_tag *)alloc_page(
      (buf_size / PAGE_SIZE) * sizeof(allocated_mem_tag));

  SYSLOG(SteveStudiosNucleusBitmapAllocator, Allocated memory tag buffer)

  allocator_unlock();

  SYSLOG(SteveStudiosNucleusBitmapAllocator, done)
}