// gdt.c - GDT (Global Descriptor Table) manager
// Created 2023/7/20 by Stephen Byrne

#include "gdt.h"

uint64_t gdt[10] = {0x0000000000000000, 0x00af9a000000ffff, 0x00cf92000000ffff,
                    0x00cf9a000000ffff, 0x00cf93000000ffff, 0x00af9b000000ffff,
                    0x00af93000000ffff, 0x00affb000000ffff, 0x00aff3000000ffff};

bool bsp_loaded_gdt = false;

// Initialize the GDT
void gdt_init() {
  asm("cli");

  init_tss(gdt);

  if (!bsp_loaded_gdt) {
    SYSLOG(nucleus_kernel, Ensure Interrupts are disabled(cli))
    SYSLOG(nucleus_kernel, Start TSS setup)

    SYSLOG(nucleus_kernel, Construct GDT register...)

    gdtr_t gdtr = {sizeof(gdt) - 1, (uint64_t)&gdt};

    SYSLOG(nucleus_kernel, lgtd)

    asm volatile("lgdt %0" : : "m"(gdtr));
    bsp_loaded_gdt = true;
    
    SYSLOG(nucleus_kernel, done)
    SYSLOG(nucleus_kernel, Reload segment registers...)

    reload_segment_registers();

    SYSLOG(nucleus_kernel, done)
  }
}
