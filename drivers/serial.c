// serial.c - Serial port initialization
// Created 2024/03/04 by Stephen Byrne

#include "serial.h"

#define PORT 0x3f8

int init_serial() {
  SYSLOG(nucleus_kernel, Flush serial ports)

  outb(PORT + 1, 0x00);
  outb(PORT + 3, 0x80);
  outb(PORT + 0, 0x03);
  outb(PORT + 1, 0x00);
  outb(PORT + 3, 0x03);
  outb(PORT + 2, 0xC7);
  outb(PORT + 4, 0x0B);
  outb(PORT + 4, 0x1E);
  outb(PORT + 0, 0xAE);

  if (inb(PORT + 0) != 0xAE) {
    SYSLOG(nucleus_kernel, Could not flush serial ports)
    return 1;
  }

  SYSLOG(nucleus_kernel, done)

  outb(PORT + 4, 0x0F);

  SYSLOG(nucleus_kernel, success)

  return 0;
}