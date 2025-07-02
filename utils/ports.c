// ports.c - In and out commands, which are useful for creating any device
// driver Created 2023/7/11 by Stephen Byrne

#include "ports.h"

void outw(uint16_t port, uint16_t d) {
  asm volatile("outw %w0, %w1" : : "a"(d), "Nd"(port));
}

void outb(uint16_t port, uint8_t d) {
  asm volatile("outb %%al, %%dx" : : "d"(port), "a"(d));
}

unsigned char inb(uint16_t port) {
  unsigned char val;
  asm volatile("inb %%dx, %%al" : "=a"(val) : "d"(port));
  return val;
}

void outl(uint16_t port, uint32_t value) {
  asm volatile("outl %%eax, %%dx" : : "d"(port), "a"(value));
}

uint32_t inl(uint16_t port) {
  uint32_t ret;
  asm volatile("inl %%dx, %%eax" : "=a"(ret) : "d"(port));
  return ret;
}

void io_wait() { outb(0x80, 0); }