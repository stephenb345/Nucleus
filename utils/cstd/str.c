// str.c - C standard library compliant string and char function replacements
// Created 2023/7/10 by Stephen Byrne

#include "str.h"

int strlen(const char *str) {
  const char *s;
  for (s = str; *s; ++s)
    ;

  return (s - str);
}

char *strcat(char *des, const char *src) {
  char *ptr = des + strlen(des);
  while (*src != '\0') {
    *ptr++ = *src++;
  }
  *ptr = '\0';
  return des;
}

int itoa(long int value, char *sp, int radix) {
  char tmp[16];
  char *tp = tmp;
  int i;
  unsigned v;

  int sign = (radix == 10 && value < 0);
  if (sign)
    v = -value;
  else
    v = (unsigned)value;

  while (v || tp == tmp) {
    i = v % radix;
    v /= radix;
    if (i < 10)
      *tp++ = i + '0';
    else
      *tp++ = i + 'a' - 10;
  }

  int len = tp - tmp;

  if (sign) {
    *sp++ = '-';
    len++;
  }

  while (tp > tmp)
    *sp++ = *--tp;

  return len;
}

int strcmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}
