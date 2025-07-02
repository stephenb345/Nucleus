// usb_uhci.h - USB 1.0 UHCI Interface Driver
// Created 2024/04/08 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../pci/pci.h"
#include <stdint.h>

typedef struct {
  uint16_t usb_cmd;
  uint16_t usb_status;
  uint16_t usb_interr_enable;
  uint16_t usb_frame_num;
  uint32_t usb_frame_list_addr : 8;
  uint8_t frame_mod_start;
  uint16_t port_status_ctrl1;
  uint16_t port_status_ctrl2;
} __attribute__((packed)) uhci_io_registers;

typedef struct {
  uint32_t next_desc;
  uint32_t stat;
  uint32_t pack_header;
  uint32_t buf_addr;
  uint32_t system_use;
} __attribute__((packed)) uhci_descriptor;

typedef struct {
  uint32_t bus;
  uint32_t slot;
  uint32_t func;
  uhci_io_registers *regs;
} __attribute__((packed)) uhci_device;

#ifdef __cplusplus
}
#endif