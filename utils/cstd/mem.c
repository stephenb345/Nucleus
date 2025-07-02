// mem.c - C standard library compliant memory function replacements
// Created 2023/7/10 by Stephen Byrne

#include "mem.h"

void mem_lock_acquire(atomic_flag *lock) {
  while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) {
    __builtin_ia32_pause();
  }
}

void mem_lock_release(atomic_flag *lock) {
  atomic_flag_clear_explicit(lock, memory_order_release);
}

extern bool using_custom_pages;
void *memcpy(void *dest, const void *src, size_t n) {
  if (using_custom_pages) {
    map_page((uint64_t)dest);
    map_page((uint64_t)src);
  }

  uint8_t *pdest = (uint8_t *)dest;
  const uint8_t *psrc = (const uint8_t *)src;

  for (size_t i = 0; i < n; i++) {
    if (using_custom_pages) {
      map_page((uint64_t)pdest + i);
      map_page((uint64_t)psrc + i);
    }
    pdest[i] = psrc[i];
  }
  return dest;
}

void *memset(void *s, int c, size_t n) {
  if (using_custom_pages)
    map_page((uint64_t)s);

  uint8_t *p = (uint8_t *)s;

  for (size_t i = 0; i < n; i++) {
    if (using_custom_pages)
      map_page((uint64_t)p + i);
    p[i] = (uint8_t)c;
  }
  return s;
}

void *memmove(void *dest, const void *src, size_t n) {
  if (using_custom_pages) {
    map_page((uint64_t)dest);
    map_page((uint64_t)src);
  }

  uint8_t *pdest = (uint8_t *)dest;
  const uint8_t *psrc = (const uint8_t *)src;

  if (src > dest) {
    for (size_t i = 0; i < n; i++) {
      if (using_custom_pages) {
        map_page((uint64_t)pdest + i);
        map_page((uint64_t)psrc + i);
      }
      pdest[i] = psrc[i];
    }
  } else if (src < dest) {
    for (size_t i = n; i > 0; i--) {
      if (using_custom_pages) {
        map_page((uint64_t)pdest + i);
        map_page((uint64_t)psrc + i);
      }
      pdest[i - 1] = psrc[i - 1];
    }
  }
  return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  if (using_custom_pages) {
    map_page((uint64_t)s1);
    map_page((uint64_t)s2);
  }

  const uint8_t *p1 = (const uint8_t *)s1;
  const uint8_t *p2 = (const uint8_t *)s2;

  for (size_t i = 0; i < n; i++) {
    if (using_custom_pages) {
      map_page((uint64_t)p2 + i);
      map_page((uint64_t)p2 + i);
    }
    if (p1[i] != p2[i]) {
      return p1[i] < p2[i] ? -1 : 1;
    }
  }
  return 0;
}

static atomic_flag mem_lock = ATOMIC_FLAG_INIT;

void allocator_lock() { mem_lock_acquire(&mem_lock); }

void allocator_unlock() { mem_lock_release(&mem_lock); }

extern void kernel_hang();
void *allocator_alloc(size_t pages) {
  void *ptr_ret = NULL;
  allocator_lock();

  ptr_ret = alloc_page(pages);
  if (ptr_ret == NULL) {
    SYSLOG(SteveStudiosPMM, Could not allocate page(Out of memory))

    allocator_unlock();
    while (alloc_page(pages) == NULL)
      asm("cli; hlt");

    return NULL;
  }

  allocator_unlock();
  return ptr_ret;
}

int allocator_free(void *ptr, size_t pages) {
  free_page(ptr, pages);
  return 0;
}