// file_descriptor.h - UNIX File descriptor manager
// Created 2024/04/06 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../../mem/allocator.h"
#include "../../utils/cstd/mem.h"

#define S_IFMT 0170000
#define S_IFIFO 0010000
#define S_IFCHR 0020000
#define S_IFDIR 0040000
#define S_IFBLK 0060000
#define S_IFREG 0100000
#define S_IFLNK 0120000
#define S_IFSOCK 0140000

typedef struct {
  unsigned int descriptor_id;
  char mode[2];
  const char *filename;
  int32_t access;
  __uint128_t offset;
  void* pending;
} file_descriptor;

bool descriptor_exists(int fd);
file_descriptor *get_descriptor(int index);

file_descriptor *mk_descriptor(const char *fname, int32_t access, char *mode);
void close_descriptor(unsigned int id);

#ifdef __cplusplus
}
#endif