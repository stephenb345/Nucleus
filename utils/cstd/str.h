// str.c - C standard library compliant string and char function replacements
// Created 2023/7/10 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "../../mem/allocator.h"

int strlen(const char *str);
char *strcat(char *dest, const char *src);
int itoa(long int value, char *sp, int radix);
int strcmp(const char* s1, const char* s2);

#ifdef __cplusplus
}
#endif