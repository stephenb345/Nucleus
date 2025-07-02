// paging.c - Paging Manager
// Created 2024/04/08 by Stephen Byrne

#include "paging.h"

uint64_t *pml4;
extern unsigned long kernel_start;

void map_page(uint64_t i) {
  uint64_t pml4_index = (i >> 39) & 0x1FF;
  uint64_t pdpt_index = (i >> 30) & 0x1FF;
  uint64_t pd_index = (i >> 21) & 0x1FF;
  uint64_t pt_index = (i >> 12) & 0x1FF;
  
  uint64_t *pdpt;
  uint64_t *pd;
  uint64_t *pt;

  if (pml4[pml4_index] == 0) {
    allocator_lock();

    pdpt = (uint64_t *)alloc_page(1);
    pml4[pml4_index] = (uint64_t)pdpt | 0b111;

    allocator_unlock();

    map_page((uint64_t)pdpt);
  }
  pdpt = (uint64_t *)(pml4[pml4_index] & 0x000FFFFFFFFFF000);

  if (pdpt[pdpt_index] == 0) {
    allocator_lock();

    pd = (uint64_t *)alloc_page(1);
    pdpt[pdpt_index] = (uint64_t)pd | 0b111;

    allocator_unlock();

    map_page((uint64_t)pd);
  }
  pd = (uint64_t *)(pdpt[pdpt_index] & 0x000FFFFFFFFFF000);

  if (pd[pd_index] == 0) {
    allocator_lock();

    pt = (uint64_t *)alloc_page(1);
    pd[pd_index] = (uint64_t)pt | 0b111;

    allocator_unlock();

    map_page((uint64_t)pt);
  }

  pt = (uint64_t *)(pd[pd_index] & 0x000FFFFFFFFFF000);
  pt[pt_index] = i | 0b111;
}

void setup_paging() {
  SYSLOG(SteveStudiosVMM, Switch from default UEFI page tables...)

  allocator_lock();

  SYSLOG(SteveStudiosVMM, Allocate pml4...)

  pml4 = (uint64_t *)alloc_page(1);
  allocator_unlock();

  SYSLOG(SteveStudiosVMM, done)

  map_page((uint64_t)pml4);

  extern unsigned long kernel_end;

  SYSLOG(SteveStudiosVMM, Map kernel...)

  uint64_t i;
  for (i = (uint64_t)&kernel_start; i <= (uint64_t)&kernel_end; i += PAGE_SIZE) {
    map_page(i);
  }

  SYSLOG(SteveStudiosVMM, done)
  SYSLOG(SteveStudiosVMM, Map GDT...)

  extern uint64_t gdt[10];
  for (i = (uint64_t)&gdt; i <= (uint64_t)&gdt + sizeof(gdt) - 1;
       i += PAGE_SIZE) {
    map_page(i);
  }

  SYSLOG(SteveStudiosVMM, done)
  SYSLOG(SteveStudiosVMM, Map IDT...)

  extern idt_entry idt[256];
  for (i = (uint64_t)&idt; i <= (uint64_t)&idt + sizeof(idt) - 1;
       i += PAGE_SIZE) {
    map_page(i);
  }

  SYSLOG(SteveStudiosVMM, done)

  extern bool paging_setup;
  paging_setup = true;

  SYSLOG(SteveStudiosVMM, Map allocator...)

  extern uint8_t *mem_buf;
  for (i = (uint64_t)mem_buf; i <= (uint64_t)mem_buf + sizeof(mem_buf); i += PAGE_SIZE) {
    map_page(i);
  }

  extern allocated_mem_tag *allocated_mem_tags;
  for (i = (uint64_t)allocated_mem_tags;
       i <= (uint64_t)allocated_mem_tags + ((boot_parameters->usable_mem_size / PAGE_SIZE) *
               sizeof(allocated_mem_tag));
       i += PAGE_SIZE) {
    map_page(i);
  }

  SYSLOG(SteveStudiosVMM, done)
  SYSLOG(SteveStudiosVMM, Map boot parameters...)

  for (i = (uint64_t)boot_parameters;
       i <= (uint64_t)boot_parameters + sizeof(boot_parameters);
       i += PAGE_SIZE) {
    map_page(i);
  }

  SYSLOG(SteveStudiosVMM, done)
  SYSLOG(SteveStudiosVMM, Map stack...)

  for (i = boot_parameters->stack_start; i <= boot_parameters->stack_end;
       i += PAGE_SIZE) {
    map_page(i);
  }

  SYSLOG(SteveStudiosVMM, done)
  SYSLOG(SteveStudiosVMM, Map framebuffer...)

  for (i = (uint64_t)(uintptr_t)boot_parameters->framebuffer;
       i <= (uint64_t)(uintptr_t)boot_parameters->framebuffer +
               boot_parameters->framebuffer_size;
       i += PAGE_SIZE) {
    map_page(i);
  }

  SYSLOG(SteveStudiosVMM, done)
  SYSLOG(SteveStudiosVMM, Map ioapics...)

  for (i = (uint32_t)(uintptr_t)boot_parameters->ioapic_entries;
       i <= (uint32_t)(uintptr_t)boot_parameters->ioapic_entries +
               (boot_parameters->ioapic_entries_size * sizeof(uint32_t));
       i += PAGE_SIZE) {
    map_page(i);
  }

  SYSLOG(SteveStudiosVMM, done)
  SYSLOG(SteveStudiosVMM, Map initramfs...)

  for (i = (uint64_t)(uintptr_t)boot_parameters->initramfs;
       i <= (uint64_t)(uintptr_t)boot_parameters->initramfs +
               boot_parameters->initramfs_size;
       i += PAGE_SIZE) {
    map_page(i);
  }

  SYSLOG(SteveStudiosVMM, done)

  SYSLOG(SteveStudiosVMM, Map UEFI pointers...)

  for (i = (uint64_t)(uintptr_t)boot_parameters->pcie_ecam_alloc_structs;
       i <= (uint64_t)(uintptr_t)boot_parameters->pcie_ecam_alloc_structs +
               boot_parameters->pcie_ecam_alloc_structs_size;
       i += PAGE_SIZE) {
    map_page(i);
  }

  map_page((uint64_t)boot_parameters->shutdown);
  map_page((uint64_t)boot_parameters->reboot);

  SYSLOG(SteveStudiosVMM, Switch page tables...)

  asm volatile("mov %0, %%cr3" ::"r"(pml4));

  SYSLOG(SteveStudiosVMM, done)
}