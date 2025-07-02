// file_descriptor.c - UNIX File descriptor manager
// Created 2024/04/06 by Stephen Byrne

#include "file_descriptor.h"

int n_descriptors = 0;
file_descriptor *file_descriptor_table = NULL;

bool descriptor_exists(int fd) {
  if (fd <= n_descriptors && fd >= 0)
    return file_descriptor_table[fd].offset != -1;
  return false;
}

file_descriptor *get_descriptor(int index) {
  int n_descriptor = index;
  for (unsigned int i = 0; i < n_descriptors; i++) {
    if (file_descriptor_table[i].descriptor_id == index) {
      n_descriptor = i;
      break;
    }
  }
  return &file_descriptor_table[n_descriptor];
}

file_descriptor *mk_descriptor(const char *fname, int32_t access, char mode[]) {
  int n_descriptor = n_descriptors;
  for (int i = 0; i < n_descriptors; i++) {
    if (file_descriptor_table[i].offset == -1) {
      n_descriptor = i;
      break;
    }
  }

  if (n_descriptor == n_descriptors) {
    if (file_descriptor_table != NULL) {
      file_descriptor_table = realloc(
          file_descriptor_table, (n_descriptors + 1) * sizeof(file_descriptor));
    } else {
      file_descriptor_table =
          malloc((n_descriptors + 1) * sizeof(file_descriptor));
    }
  }

  file_descriptor desc;

  desc.descriptor_id = n_descriptors;
  desc.access = access;
  
  desc.filename = malloc(strlen(fname));

  memcpy((void*)desc.filename, fname, strlen(fname));
  memcpy(desc.mode, mode, sizeof(desc.mode));

  file_descriptor_table[n_descriptor] = desc;

  n_descriptors++;

  return &file_descriptor_table[n_descriptor];
}

void close_descriptor(unsigned int id) {
  for (unsigned int i = 0; i < n_descriptors; i++) {
    if (file_descriptor_table[i].descriptor_id == id) {
      file_descriptor_table[i].offset = -1;

      free((void*)file_descriptor_table[i].filename);
      file_descriptor_table[i].filename = NULL;
      
      n_descriptors--;
      break;
    }
  }
  if (id == n_descriptors && n_descriptors > 0)
    file_descriptor_table =
        realloc(file_descriptor_table, n_descriptors * sizeof(file_descriptor));
  else if (n_descriptors == 0)
    file_descriptor_table[0].offset = -1;
}