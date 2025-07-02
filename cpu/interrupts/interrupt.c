// interrupt.c - Interrupt handler
// Created 2023/7/21 by Stephen Byrne

#include "interrupt.h"

extern bool nvme_ready;
extern bool ide_ready;

extern void write_register(uint32_t reg, uint32_t value);

uint32_t interrupts_to_wait = 0;

// Called from (cpu/interrupts/asm/irq_helper.asm)
void handle_interrupt(int irq_n) {
  switch (irq_n) {
  case 0:
    if (interrupts_to_wait > 0)
      interrupts_to_wait--;
    update_thread_time();
    break;
  case 1:
    if (inb(0x60) == 0xE0)
      last_key = 0xE0;
    handle_keyboard();
    break;
  case 14:
  case 15:
    ide_ready = true;
    break;
  case 16:
    nvme_ready = true;
    break;
  }
  write_register(0xB0, 0);
}