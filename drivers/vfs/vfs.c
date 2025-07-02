// vfs.c - Virtual Filesystem
// Created 2023/11/17 by Stephen Byrne

#include "vfs.h"

bool vmkfile(char *path, char *contents) {
  uint16_t permissions = 000000000;

  ssfs_del_file_or_link(path);
  return ssfs_mk_file(path, contents, permissions);
}

bool vmkdir(char *path) {
  uint16_t permissions = 000000000;
  return ssfs_mk_dir(path, permissions);
}

__uint128_t fsize = 0;
char *vrdfile(char *path) {
  file_response resp = ssfs_read_file(path);
  fsize = resp.size;
  return resp.content;
}