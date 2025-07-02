// pci.h - PCI Driver
// Created 2023/11/12 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../../mem/allocator.h"
#include "../../utils/cstd/mem.h"
#include "../../utils/display/io.h"
#include "../../utils/ports.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint32_t vendor;
  uint32_t device;
  uint32_t func;
  uint32_t slot;
  uint32_t bus;
  uint8_t type;
} pci_device;

typedef struct {
  uint32_t msg_control;
  uint64_t msg_address;
  uint64_t msg_data;
} msi_capability;

typedef struct {
  uint32_t msg_control;
  uint64_t table_addr;
  uint64_t pba_addr;
} msix_capability;

bool is_pcie(uint32_t bus, uint32_t slot, uint32_t func);

void add_pci_device(pci_device pdev);
extern void pcie_probe();
void pci_probe();

bool enable_msix(uint8_t bus, uint8_t slot, uint8_t func, uint8_t vector,
                 uint8_t irq_vector);
bool enable_msi(uint8_t bus, uint8_t slot, uint8_t func, uint8_t irq_vector);

void pci_or_pcie_write_dword(uint8_t bus, uint8_t slot, uint8_t func,
                             uint8_t offset, uint32_t value);
uint32_t pci_or_pcie_read_dword(uint8_t bus, uint8_t slot, uint8_t func,
                                uint8_t offset);

void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                     uint32_t value);
uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func,
                        uint8_t offset);
uint16_t getClassId(uint8_t bus, uint8_t device, uint8_t function);
uint16_t getSubClassId(uint8_t bus, uint8_t device, uint8_t function);

#ifdef __cplusplus
}
#endif