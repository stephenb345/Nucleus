// hal.c - Hardware Abstraction Layer
// Created 2024/03/29 by Stephen Byrne

#include "hal.h"

hal_disk_interface get_pci_disk_interface(uint32_t bus, uint32_t slot,
                                          uint32_t func) {

  if (getSubClassId(bus, slot, func) == 0x06 &&
      getClassId(bus, slot, func) == 0x01)
    return AHCI_SATA;
  if (getSubClassId(bus, slot, func) == 0x08 &&
      getClassId(bus, slot, func) == 0x01)
    return NVME;
  if (getSubClassId(bus, slot, func) == 0x01 &&
      getClassId(bus, slot, func) == 0x01) {
    ide_initialize(pci_or_pcie_read_dword(bus, slot, func, 0x10),
                   pci_or_pcie_read_dword(bus, slot, func, 0x14),
                   pci_or_pcie_read_dword(bus, slot, func, 0x18),
                   pci_or_pcie_read_dword(bus, slot, func, 0x1C),
                   pci_or_pcie_read_dword(bus, slot, func, 0x20), bus, slot,
                   func);
  }

  int type = -1;
  for (int i = 0; i < 4; i++) {
    if (ide_devices[i].bus == bus && ide_devices[i].slot == slot &&
        ide_devices[i].func == func) {
      type = ide_devices[i].Type;
      break;
    }
  }

  if (type != -1) {
    if (getSubClassId(bus, slot, func) == 0x01 &&
        getClassId(bus, slot, func) == 0x01 && type == 0x01)
      return ATAPI;
    if (getSubClassId(bus, slot, func) == 0x01 &&
        getClassId(bus, slot, func) == 0x01 && type == 0x00)
      return IDE;
  }
  return NIL;
}

hal_driver_response hal_call(hal_driver_func func, hal_driver_args args) {
  hal_driver_response resp;
  switch (func) {
  case HAL_AHCI_READ:
    resp.b = hal_disk_read(AHCI_SATA, args.startl, args.count, args.buf);
    break;

  case HAL_AHCI_WRITE:
    resp.b = hal_disk_read(AHCI_SATA, args.startl, args.count, args.buf);
    break;
  }
  return resp;
}