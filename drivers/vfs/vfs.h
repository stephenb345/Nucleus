// vfs.h - Virtual Filesystem
// Created 2023/11/17 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../filesystems/ssfs.h"

bool vmkfile(char *, char *);
bool vmkdir(char *);
char *vrdfile(char *);

#ifdef __cplusplus
}
#endif