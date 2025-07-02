// tss.h TSS Driver
// Created 2024/05/10 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../utils/bytes.h"
#include "../utils/cstd/memalloc.h"

typedef struct {
  uint32_t reserved_0;
  uint32_t rsp0_l;
  uint8_t rsp0_h;
  uint8_t rsp1_l;
  uint32_t rsp1_h;
  uint32_t rsp2_l;
  uint8_t rsp2_h;
  uint8_t reserved_1;
  uint32_t reserved_2;
  uint32_t ist1_l;
  uint8_t ist1_h;
  uint32_t ist2_l;
  uint8_t ist2_h;
  uint32_t ist3_l;
  uint8_t ist3_h;
  uint32_t ist4_l;
  uint8_t ist4_h;
  uint32_t is5_l;
  uint8_t ist5_h;
  uint32_t ist6_l;
  uint8_t ist6_h;
  uint32_t ist7_l;
  uint8_t ist7_h;
  uint8_t reserved_3;
  uint32_t reserved_4;
  uint32_t iopb;
} tss_entry_t;

typedef struct {
  uint32_t limit_low : 16;
  uint32_t base_low : 24;
  uint32_t accessed : 1;
  uint32_t read_write : 1;
  uint32_t conforming : 1;
  uint32_t code : 1;
  uint32_t code_data_segment : 1;
  uint32_t DPL : 2;
  uint32_t present : 1;
  uint32_t limit_high : 4;
  uint32_t available : 1;
  uint32_t long_mode : 1;
  uint32_t big : 1;
  uint32_t gran : 1;
  uint32_t base_high : 8;
} gdt_entry_bits;

void update_stack(uint64_t stack);
void init_tss(uint64_t m_gdt[]);

#ifdef __cplusplus
}
#endif