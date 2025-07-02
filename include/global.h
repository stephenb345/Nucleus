// global.h - Global defines which are used many times in kernel code
// Created 2023/11/10 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SECTOR_SIZE 512

// #if defined(__x86_64__) || defined(_M_X64)
#define X86_64
/* #elif defined(__aarch64__) || defined(_M_ARM64)
#define AARCH64
#else
#error "Unsupported architecture; Only 64-bit x86 and arm architectures are supported!"
#endif 
*/

#define SYSLOG(prefix, str)                                                    \
  print((const char *)#prefix);                                                \
  print(": ");                                                                 \
  print((const char *)__func__);                                               \
  print(":");                                                                  \
  print_int((int)__LINE__);                                                    \
  print(": ");                                                                 \
  println((const char *)#str);

typedef struct {
  uint8_t start_bus;
  uint8_t end_bus;
  uint64_t ecam_base_address;
} NUCLEUS_ECAM_ALLOC_STRUCT;

typedef struct {
  uint64_t stack_start;
  uint64_t stack_end;
  uint32_t framebuffer_size;
  uint32_t *framebuffer;
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint64_t ioapic_entries_size;
  uint32_t *ioapic_entries;
  uint64_t lapic_ids_size;
  uint8_t *lapic_ids;
  uint64_t mem_size;
  uint64_t usable_mem_size;
  uint64_t pcie_ecam_alloc_structs_size;
  uint64_t pcie_ecam_alloc_structs_len;
  NUCLEUS_ECAM_ALLOC_STRUCT *pcie_ecam_alloc_structs;
  void (*shutdown)(void);
  void (*reboot)(void);
  uint64_t initramfs_size;
  void *initramfs;
} NUCLEUS_PARAMETERS;

#ifdef __cplusplus
}
#endif