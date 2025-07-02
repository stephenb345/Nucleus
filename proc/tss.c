// tss.c - TSS Driver
// Created 2024/05/10 by Stephen Byrne

#include "tss.h"

static gdt_entry_bits tss_gdt[6];
tss_entry_t tss_entry;

void write_tss(gdt_entry_bits *g) {
  uint32_t base = (uint32_t)(uintptr_t)&tss_entry;
  uint32_t limit = sizeof(tss_entry);

  g->limit_low = limit;
  g->base_low = base;
  g->accessed = 1;
  g->read_write = 0;
  g->conforming = 0;
  g->code = 1;
  g->code_data_segment = 0;
  g->DPL = 0;
  g->present = 1;
  g->limit_high = (limit & (0xf << 16)) >> 16;
  g->available = 0;
  g->long_mode = 0;
  g->big = 0;
  g->gran = 0;
  g->base_high = (base & (0xff << 24)) >> 24;

  memset(&tss_entry, 0, sizeof(tss_entry));

  tss_entry.rsp0_l = get_low_bytes_32(boot_parameters->stack_end);
  tss_entry.rsp0_h = get_high_bytes_32(boot_parameters->stack_end);
}

void update_stack(uint64_t stack) {
  tss_entry.rsp0_l = get_low_bytes_32(boot_parameters->stack_end);
  tss_entry.rsp0_h = get_high_bytes_32(boot_parameters->stack_end);
}

void init_tss(uint64_t m_gdt[]) {
  gdt_entry_bits *ring3_code = &tss_gdt[3];
  gdt_entry_bits *ring3_data = &tss_gdt[4];

  ring3_code->limit_low = 0xFFFF;
  ring3_code->base_low = 0;
  ring3_code->accessed = 0;
  ring3_code->read_write = 1;
  ring3_code->conforming = 0;
  ring3_code->code = 1;
  ring3_code->code_data_segment = 1;
  ring3_code->DPL = 3;
  ring3_code->present = 1;
  ring3_code->limit_high = 0xF;
  ring3_code->available = 1;
  ring3_code->long_mode = 0;
  ring3_code->big = 1;
  ring3_code->gran = 1;
  ring3_code->base_high = 0;

  *ring3_data = *ring3_code;
  ring3_data->code = 0;

  write_tss(&tss_gdt[5]);

  m_gdt[9] = (uint64_t)(uintptr_t)&tss_gdt;
}