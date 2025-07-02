// usb_ohci.c - USB 1.0 OHCI Interface Driver
// Created 2024/05/09 by Stephen Byrne

#include "usb_ohci.h"

extern uint32_t devs;
extern pci_device *devices;

uint32_t ohci_devs;
uhci_device ochi_devices[127];

extern uint32_t pci_or_pcie_read_dword(uint8_t bus, uint8_t slot, uint8_t func,
                                uint8_t offset);

void ohci_probe() {
  int index = ohci_devs;
  for (int i = ohci_devs; i < devs; i++) {
    uint32_t bus = devices[i].bus;
    uint32_t slot = devices[i].slot;
    uint32_t func = devices[i].func;

    if (getClassId(bus, slot, func) == 0x0C &&
        getSubClassId(bus, slot, func) == 0x03 &&
        (getClassId(bus, slot, func) & 0xFF) == 0x10) {
      uhci_device dev;
      dev.bus = bus;
      dev.slot = slot;
      dev.func = func;
      dev.regs = (ohci_io_registers *)(uintptr_t)pci_or_pcie_read_dword(
                           bus, slot, func, 0x10);

      ochi_devices[index] = dev;
      index++;
    }
  }
}