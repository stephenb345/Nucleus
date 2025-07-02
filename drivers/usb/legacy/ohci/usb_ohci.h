// usb_ohci.h - USB 1.0 OHCI Interface Driver
// Created 2024/05/09 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../pci/pci.h"
#include <stdint.h>

typedef struct {
  uint32_t hc_revision;
  uint32_t hc_control;
  uint8_t hc_command_status;
  uint8_t hc_interrupt_status;
  uint32_t hc_interrupt_enable;
  uint32_t hc_interrupt_disable;
  uint8_t hc_hcaa;
  uint8_t hc_period_current_ed;
  uint32_t hc_control_head_ed;
  uint32_t hc_control_current_ed;
  uint8_t hc_bulk_head_ed;
  uint8_t hc_bulk_current_ed;
  uint32_t hc_done_head;
  uint32_t hc_fm_interval;
  uint8_t hc_fm_remaining;
  uint8_t hc_fm_number;
  uint32_t hc_periodic_start;
  uint32_t hc_ls_threshold;
  uint8_t hc_rh_descriptor_a;
  uint8_t hc_rh_descriptor_b;
  uint32_t hc_rh_status;
} __attribute__((packed)) ohci_io_registers;

typedef struct {
  uint32_t control;
  uint32_t tail_td_addr;
  uint8_t head_td_addr;
  uint8_t next_ed_addr;
} __attribute__((packed)) ohci_endpoint_descriptor;

typedef struct {
  uint32_t control;
  uint32_t first_byte_addr;
  uint8_t next_td_addr;
  uint8_t last_byte_addr;
} __attribute__((packed)) ohci_transfer_descriptor;

typedef struct {
  uint32_t bus;
  uint32_t slot;
  uint32_t func;
  ohci_io_registers *regs;
} __attribute__((packed)) uhci_device;

#ifdef __cplusplus
}
#endif