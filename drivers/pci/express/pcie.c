// pcie.c - PCIe Driver
// Created 2024/04/07 by Stephen Byrne

#include "pcie.h"

NUCLEUS_ECAM_ALLOC_STRUCT *get_pcie_group(uint8_t bus) {
  for (uint64_t i = 0; i < boot_parameters->pcie_ecam_alloc_structs_len; i++) {
    if (bus >= boot_parameters->pcie_ecam_alloc_structs[i].start_bus &&
        bus <= boot_parameters->pcie_ecam_alloc_structs[i].end_bus)
      return &boot_parameters->pcie_ecam_alloc_structs[i];
  }
  return NULL;
}

void pcie_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                uint32_t value) {
  NUCLEUS_ECAM_ALLOC_STRUCT *group = get_pcie_group(bus);
  if (group == NULL)
    return;

  uint64_t address =
      group->ecam_base_address +
      (((bus - group->start_bus) << 20) | (slot << 15) | (func << 12)) + offset;
  if (address % sizeof(uint64_t) != 0)
    return;

  volatile uint32_t *ptr = (volatile uint32_t *)(uintptr_t)address;
  map_page((uint64_t)ptr);

  *ptr = value;
}

uint32_t pcie_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
  NUCLEUS_ECAM_ALLOC_STRUCT *group = get_pcie_group(bus);
  if (group == NULL)
    return 0xFFFF;

  uint64_t address =
      group->ecam_base_address +
      (((bus - group->start_bus) << 20) | (slot << 15) | (func << 12)) + offset;
  if (address % sizeof(uint64_t) != 0)
    return 0xFFFF;

  volatile uint32_t *ptr = (volatile uint32_t *)(uintptr_t)address;
  map_page((uint64_t)ptr);
  
  return *ptr;
}

void pcie_probe() {
  SYSLOG(SteveStudiosPCIeDriver, Probe PCI...)

  for (int bus = 0; bus < 256; bus++) {
    for (int slot = 0; slot < 32; slot++) {
      for (int function = 0; function < 8; function++) {
        uint16_t vendor =
            (uint16_t)(pcie_read(bus, slot, function, 0x0) & 0xFFFF);
        if (vendor == 0xFFFF)
          continue;

        uint16_t device =
            (uint16_t)((pcie_read(bus, slot, function, 2) >> 16) & 0xFFFF);

        pci_device pdev;
        pdev.vendor = vendor;
        pdev.device = device;
        pdev.func = function;
        pdev.bus = bus;
        pdev.slot = slot;
        pdev.type = 1;

        add_pci_device(pdev);
      }
    }
  }
}