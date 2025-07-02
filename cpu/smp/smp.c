// smp.c - SMP Support
// Created 2024/7/22 by Stephen Byrne

#include "smp.h"

volatile uint8_t active_aps = 0;
uint8_t bspid, bspdone = 0;

extern NUCLEUS_PARAMETERS *boot_parameters;

extern void ap_trampoline();
extern void ap_trampoline_long_mode();

extern uint64_t *pml4;
extern uint64_t gdt[10];

void start_smp() {
  asm volatile("mov $1, %%eax; cpuid; shrl $24, %%ebx;" : "=b"(bspid) : :);
  memcpy((void *)0x8000, &ap_trampoline, PAGE_SIZE);

  uint64_t lapic_ptr = (uint64_t)get_base();
  map_page(lapic_ptr);

  map_page(0x4000);
  *(volatile uint64_t *)0x4000 = (uint64_t)pml4;

  memcpy((void *)0x5000, &ap_trampoline_long_mode, PAGE_SIZE);
  for (int i = 0; i < boot_parameters->lapic_ids_size; i++) {
    if (boot_parameters->lapic_ids[i] == bspid)
      continue;

    uint64_t ap_stack_start = (uint64_t)malloc(131072);
    uint64_t ap_stack_end = ap_stack_start + 131072 - 16;
    ap_stack_end -= ap_stack_end % 16;

    map_page(0x6000);
    for (uint64_t k = ap_stack_start; k < ap_stack_end; k += PAGE_SIZE) {
      map_page(k);
    }

    *(volatile uint64_t *)0x6000 = ap_stack_end;

    gdtr_t gdtr = {sizeof(gdt) - 1, (uint64_t)&gdt};
    memcpy((void *)0x7000, &gdtr, sizeof(gdtr_t));

    write_register(0x280, 0);

    write_register(0x310, (read_register(0x310) & 0x00ffffff) | (i << 24));
    write_register(0x300, (read_register(0x300) & 0xfff00000) | 0x00C500);

    do {
      asm volatile("pause" : : : "memory");
    } while (read_register(0x300) & (1 << 12));

    write_register(0x310, (read_register(0x310) & 0x00ffffff) | (i << 24));
    write_register(0x300, (read_register(0x300) & 0xfff00000) | 0x008500);

    do {
      asm volatile("pause" : : : "memory");
    } while (read_register(0x300) & (1 << 12));

    msleep(10);

    for (int j = 0; j < 2; j++) {
      write_register(0x280, 0);

      write_register(0x310, (read_register(0x310) & 0x00ffffff) | (i << 24));
      write_register(0x300, (read_register(0x300) & 0xfff0f800) | 0x000608);

      usleep_hundreds(2);

      do {
        asm volatile("pause" : : : "memory");
      } while (read_register(0x300) & (1 << 12));
    }
  }
  bspdone = 1;
}

void ap_main(uint8_t id) {
  for (;;) {
    if (task != NULL && task->processor == id && task->vacant) {
      task->vacant = false;
      create_thread((char *)task->name, task->entry);
      continue;
    }
    asm("hlt");
  }
}