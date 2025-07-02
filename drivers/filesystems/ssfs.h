// ssfs.h - Steve Studios Filesystem Driver
// Created 2023/11/12 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "utils/ssfs_utils.h"

typedef struct {
  char *content;
  long size;
} file_response;

file_response ssfs_read_file(char path[]);
bool ssfs_del_file_or_link(char path[]);
bool ssfs_del_dir(char path[]);

bool ssfs_mk_symlink(char src_path[], char target_path[]);
bool ssfs_mk_dir(char path[], uint16_t permissions);
bool ssfs_mk_file(char path[], char data[], uint16_t permissions);

void ssfs_reload_driver(char *device);

#ifdef __cplusplus
}
#endif