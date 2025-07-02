// apic.c - APIC setup helper
// Created 2023/7/21 by Stephen Byrne

#include "apic.h"

bool paging_setup = false;

static atomic_flag mem_lock_apic = ATOMIC_FLAG_INIT;

void apic_lock() { mem_lock_acquire(&mem_lock_apic); }

void apic_unlock() { mem_lock_release(&mem_lock_apic); }

void cpuid(int code, uint32_t *a, uint32_t *d) {
  asm volatile("cpuid" : "=a"(*a), "=d"(*d) : "a"(code) : "ecx", "ebx");
}

bool check_apic() {
  uint32_t eax, edx;
  cpuid(1, &eax, &edx);
  return edx & 0x100;
}

void get_msr(uint32_t msr, uint32_t *lo, uint32_t *hi) {
  asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void set_msr(uint32_t msr, uint32_t lo, uint32_t hi) {
  asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

void set_base(uintptr_t apic) {
  uint32_t edx = 0;
  uint32_t eax = (apic & 0xfffff0000) | 0x800;

  set_msr(0x1B, eax, edx);
}

uintptr_t get_base() {
  uint32_t eax, edx;
  get_msr(0x1B, &eax, &edx);

  return (eax & 0xfffff000);
}

uint32_t read_register(uint32_t reg) {
  if (paging_setup)
    map_page((uint64_t)(get_base() + reg));
  return *((volatile uint32_t *)(get_base() + reg));
}

void write_register(uint32_t reg, uint32_t value) {
  if (paging_setup)
    map_page((uint64_t)(get_base() + reg));
  *((volatile uint32_t *)(get_base() + reg)) = value;
}

extern uint32_t get_processor();
extern NUCLEUS_PARAMETERS *boot_parameters;
uint32_t ioapic_read(void *ioapicaddr, uint32_t reg) {
  uint32_t volatile *ioapic =
      (uint32_t volatile *)((uintptr_t)boot_parameters->ioapic_entries[0]);
  ioapic[0] = (reg & 0xff);
  return ioapic[4];
}

void ioapic_write(void *ioapicaddr, uint32_t reg, uint32_t value) {
  uint32_t volatile *ioapic = (uint32_t volatile *)ioapicaddr;
  if (paging_setup)
    map_page((uint64_t)ioapic);

  ioapic[0] = (reg & 0xff);
  ioapic[4] = value;
}

void route_timer() {
  uint32_t *ioapic_base =
      (uint32_t *)(uintptr_t)boot_parameters->ioapic_entries[0];
  int entry_index = IOAPIC_REDTBL + 4;

  red_tbl_entry entry;
  memset(&entry, 0, sizeof(red_tbl_entry));

  entry.vector = 32;
  entry.delivery_mode = 0;
  entry.destination_mode = 0;
  entry.polarity = 0;
  entry.trigger_mode = 0;
  entry.interrupt_mask = 0;
  entry.destination_field = get_processor();

  ioapic_write(ioapic_base, entry_index, *(volatile uint32_t *)&entry);
  ioapic_write(ioapic_base, entry_index + 1,
               *(volatile uint32_t *)((uint64_t)&entry + 1));

  SYSLOG(SteveStudiosAPICDriver, PIT done)
}

void route_nvme() {
  uint32_t *ioapic_base =
      (uint32_t *)(uintptr_t)boot_parameters->ioapic_entries[0];
  int entry_index = IOAPIC_REDTBL + 32;

  red_tbl_entry entry;
  memset(&entry, 0, sizeof(red_tbl_entry));

  entry.vector = 48;
  entry.delivery_mode = 0;
  entry.destination_mode = 0;
  entry.polarity = 0;
  entry.trigger_mode = 0;
  entry.interrupt_mask = 0;
  entry.destination_field = get_processor();

  ioapic_write(ioapic_base, entry_index, *(volatile uint32_t *)&entry);
  ioapic_write(ioapic_base, entry_index + 1,
               *(volatile uint32_t *)((uint64_t)&entry + 1));

  SYSLOG(SteveStudiosAPICDriver, NVMe done)
}

void route_keyboard() {
  uint32_t *ioapic_base =
      (uint32_t *)(uintptr_t)boot_parameters->ioapic_entries[0];
  int entry_index = IOAPIC_REDTBL + 2;

  red_tbl_entry entry;
  memset(&entry, 0, sizeof(red_tbl_entry));

  entry.vector = 33;
  entry.delivery_mode = 0;
  entry.destination_mode = 0;
  entry.polarity = 0;
  entry.trigger_mode = 0;
  entry.interrupt_mask = 0;
  entry.destination_field = get_processor();

  ioapic_write(ioapic_base, entry_index, *(volatile uint32_t *)&entry);
  ioapic_write(ioapic_base, entry_index + 1,
               *(volatile uint32_t *)((uint64_t)&entry + 1));

  SYSLOG(SteveStudiosAPICDriver, Keyboard done)
}

uint64_t get_segment_base(uint64_t gdt_entry) {
  uint64_t base = (gdt_entry & 0xFFFFFF) | ((gdt_entry >> 16) & 0xFF0000) |
                  ((gdt_entry >> 32) & 0xFF00000000);
  return base;
}

void enable_syscalls() {
  extern void *sys_call();
  extern uint64_t gdt[10];

  uint64_t k_cs = get_segment_base(gdt[1]);
  uint64_t u_cs = get_segment_base(gdt[3]);


  SYSLOG(SteveStudiosAPICDriver, Setup STAR MSR...)

  set_msr(0xC0000081, (uint32_t)((uint64_t)sys_call & 0xFFFFFFFF),
          (k_cs & 0xFFFFU) | ((u_cs & 0xFFFFU) << 16));
          
  SYSLOG(SteveStudiosAPICDriver, done)
  SYSLOG(SteveStudiosAPICDriver, Setup LSTAR MSR...)

  set_msr(0xC0000082, (uint32_t)((uint64_t)sys_call & 0xFFFFFFFF), (uint32_t)((uint64_t)sys_call >> 32));

  SYSLOG(SteveStudiosAPICDriver, done)
  SYSLOG(SteveStudiosAPICDriver, Setup CSTAR MSR...)

  set_msr(0xC0000083, (uint32_t)((uint64_t)sys_call & 0xFFFFFFFF), (uint32_t)((uint64_t)sys_call >> 32));

  SYSLOG(SteveStudiosAPICDriver, done)
}

void enable_apic() {
  apic_lock();

  SYSLOG(SteveStudiosAPICDriver, Set base...)

  apic_unlock();

  set_base(get_base());

  apic_lock();

  SYSLOG(SteveStudiosAPICDriver, Base is set)

  write_register(0xF0, read_register(0xF0) | 0x100);

  SYSLOG(SteveStudiosAPICDriver, Route IRQs with ioapic)

  apic_unlock();

  route_timer();
  route_keyboard();
  route_nvme();

  apic_lock();

  SYSLOG(SteveStudiosAPICDriver, done)

  SYSLOG(SteveStudiosAPICDriver, Setup syscall and sysret instructions...)
    
  apic_unlock();

  enable_syscalls();

  apic_lock();

  SYSLOG(SteveStudiosAPICDriver, syscall and sysret instructions are ready to be used)
  
  apic_unlock();
}
