// apic.h - APIC setup helper
// Created 2023/7/21 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/global.h"
#include "../utils/cstd/mem.h"
#include "../utils/display/io.h"
#include "../handlers/handler_syscall.h"

#include <stdbool.h>
#include <stdint.h>

#define IOAPIC_REDTBL 0x10

typedef struct {
  uint8_t vector;
  uint8_t delivery_mode : 2;
  uint8_t destination_mode : 1;
  uint8_t readonly_1 : 1;
  uint8_t polarity : 1;
  uint8_t readonly_2 : 1;
  uint8_t trigger_mode : 1;
  uint8_t interrupt_mask : 1;
  uint64_t reserved : 38;
  uint8_t destination_field;
} __attribute__((packed)) red_tbl_entry;

uintptr_t get_base();
void enable_apic();

uint32_t read_register(uint32_t reg);
void write_register(uint32_t reg, uint32_t value);

#ifdef __cplusplus
}
#endif