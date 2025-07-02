// usb_ehci.h - USB 2.0 EHCI Interface Driver
// Created 2024/05/09 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../../pci/pci.h"
#include <stdint.h>

typedef struct {
  uint8_t cap_length;
  uint8_t reserved;
  uint16_t hci_version;
  uint32_t hcs_params;
  uint8_t hcc_params;
  uint8_t comp_route_desc;
} __attribute__((packed)) ehci_capability_registers;

typedef struct {
  uint32_t usb_cmd;
  uint32_t usb_status;
  uint8_t usb_int_enable;
  uint8_t usb_frame_index;
  uint32_t segment_selector_4G; 
  uint32_t frame_list_base_addr;
  uint16_t async_list_addr;
  uint32_t configured_flag;
  uint32_t port_status_ctrl[1];
} __attribute__((packed)) ehci_operation_registers;

typedef struct {
  uint32_t bus;
  uint32_t slot;
  uint32_t func;
  ehci_capability_registers *regs;
  ehci_operation_registers *regs;
} __attribute__((packed)) ehci_device;

#ifdef __cplusplus
}
#endif