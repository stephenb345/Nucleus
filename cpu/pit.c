// pit.c - x86 PIT Driver
// Created 2024/05/12 by Stephen Byrne

#include "pit.h"

void reload_pit(uint16_t frequency) {
  outb(PIT_COMMAND_PORT, 0x36);
  outb(PIT_CHANNEL_0_DATA_PORT, frequency & 0xFF);
  outb(PIT_CHANNEL_0_DATA_PORT, (frequency >> 8) & 0xFF);
}

extern uint32_t interrupts_to_wait;

void msleep(uint32_t milliseconds) {
  interrupts_to_wait = milliseconds;
  while (interrupts_to_wait > 0)
    asm("hlt");
}

void usleep_hundreds(uint32_t hundreds_microseconds) {
  reload_pit(119);
  
  interrupts_to_wait = hundreds_microseconds;
  while (interrupts_to_wait)
     asm("hlt");

  reload_pit(1193);
}