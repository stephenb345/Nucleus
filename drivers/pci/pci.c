// pci.c - PCI Driver
// Created 2023/11/12 by Stephen Byrne

#include "pci.h"

uint32_t devs = 0;
pci_device devices[8192];

extern void pcie_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                       uint32_t value);
extern uint16_t pcie_read(uint8_t bus, uint8_t slot, uint8_t func,
                          uint8_t offset);

bool is_pcie(uint32_t bus, uint32_t slot, uint32_t func) {
  int index = -1;
  bool should_break = false;

  for (int i = 0; i < devs; i++) {
    if (devices[i].bus != bus)
      continue;
    for (int j = 0; j < devs; j++) {
      if (devices[j].slot != slot)
        continue;
      for (int k = 0; k < devs; k++) {
        if (devices[k].func == func) {
          index = k;
          should_break = true;
          break;
        }
      }
      if (should_break)
        break;
    }
    if (should_break)
      break;
  }
  if (index == -1)
    return false;
  else
    return devices[index].type;
}

void add_pci_device(pci_device pdev) {
  for (int i = 0; i < devs; i++) {
    if (devices[i].vendor == pdev.vendor && devices[i].device == pdev.device &&
        devices[i].slot == pdev.slot && devices[i].bus == pdev.bus &&
        devices[i].func == pdev.func)
      return;
  }

  devices[devs] = pdev;
  devs++;

  if (is_pcie(pdev.bus, pdev.slot, pdev.func)) {
    SYSLOG(SteveStudiosPCIeDriver, PCIe Device Found)
  } else {
    SYSLOG(SteveStudiosLegacyPCIDriver, PCI Device Found)
  }
}

void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                     uint32_t value) {
  uint32_t address;
  uint32_t lbus = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;

  address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                       (offset & 0xfc) | ((uint32_t)0x80000000));
  outl(0xCF8, address);
  outl(0xCFC, value);
}

void pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                    uint16_t value) {
  uint32_t address;
  uint32_t lbus = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;

  address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                       (offset & 0xFC) | 0x80000000);

  outl(0xCF8, address);
  outw(0xCFC + (offset & 2), value);
}

uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func,
                        uint8_t offset) {
  uint32_t address;
  uint32_t lbus = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;

  address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                       (offset & 0xfc) | ((uint32_t)0x80000000));
  outl(0xCF8, address);
  return inl(0xCFC);
}

uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func,
                       uint8_t offset) {
  uint32_t address;
  uint32_t lbus = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;
  uint16_t tmp = 0;

  address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                       (offset & 0xfc) | ((uint32_t)0x80000000));
  outl(0xCF8, address);
  tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
  return tmp;
}

void pci_or_pcie_write_dword(uint8_t bus, uint8_t slot, uint8_t func,
                             uint8_t offset, uint32_t value) {
  if (!is_pcie(bus, slot, func))
    pci_write_dword(bus, slot, func, 0x24, value);
  else
    pcie_write(bus, slot, func, 0x24, value);
}

uint16_t pci_or_pcie_read_word(uint8_t bus, uint8_t slot, uint8_t func,
                               uint8_t offset) {
  return !is_pcie(bus, slot, func) ? pci_read_word(bus, slot, func, 0x24)
                                   : (uint16_t)pcie_read(bus, slot, func, 0x24);
}

uint32_t pci_or_pcie_read_dword(uint8_t bus, uint8_t slot, uint8_t func,
                                uint8_t offset) {
  return !is_pcie(bus, slot, func) ? pci_read_dword(bus, slot, func, 0x24)
                            : pcie_read(bus, slot, func, 0x24);
}

uint8_t find_msix_capability(uint8_t bus, uint8_t slot, uint8_t func) {
  uint8_t cap_pointer = pci_or_pcie_read_word(bus, slot, func, 0x34);
  while (cap_pointer != 0) {
    uint8_t cap_id =
        (uint8_t)pci_or_pcie_read_word(bus, slot, func, cap_pointer);
    if (cap_id == 0x11)
      return cap_pointer;
    cap_pointer = pci_or_pcie_read_word(bus, slot, func, cap_pointer + 1);
  }
  return 0;
}

uint8_t find_msi_capability(uint8_t bus, uint8_t slot, uint8_t func) {
  uint8_t cap_pointer = pci_or_pcie_read_word(bus, slot, func, 0x34);
  while (cap_pointer != 0) {
    uint8_t cap_id =
        (uint8_t)pci_or_pcie_read_word(bus, slot, func, cap_pointer);
    if (cap_id == 0x05)
      return cap_pointer;
    cap_pointer = pci_or_pcie_read_word(bus, slot, func, cap_pointer + 1);
  }
  return 0;
}

extern uintptr_t get_base();
uint64_t arch_msi_address(uint64_t *data, uint8_t vector, uint8_t processor,
                          uint8_t edgetrigger, uint8_t deassert) {
  *data = (vector & 0xFF) | (edgetrigger == 1 ? 0 : (1 << 15)) |
          (deassert == 1 ? 0 : (1 << 14));
  return ((unsigned int)get_base() | (processor << 12));
}

void msi_cpuid(int code, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
  asm volatile("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(code) :);
}

uint32_t get_processor() {
  uint32_t eax, ebx, ecx, edx;

  msi_cpuid(1, &eax, &ebx, &ecx, &edx);
  uint32_t local_apic_id = (ebx >> 24) & 0xFF;

  return local_apic_id;
}

bool enable_msix(uint8_t bus, uint8_t slot, uint8_t func, uint8_t msi_vector,
                 uint8_t irq_vector) {
  msix_capability msix_cap;

  uint8_t cap_ptr = find_msix_capability(bus, slot, func);
  if (cap_ptr == 0)
    return true;

  msix_cap.msg_control = pci_or_pcie_read_dword(bus, slot, func, cap_ptr);
  msix_cap.table_addr = pci_or_pcie_read_dword(bus, slot, func, cap_ptr + 0x4);

  msix_cap.msg_control |= 0x80000000;

  pci_or_pcie_write_dword(bus, slot, func, cap_ptr, msix_cap.msg_control);

  uint32_t table_bir = msix_cap.table_addr & 0x7;

  uint32_t bar0 =
      pci_or_pcie_read_dword(bus, slot, func, 0x10 + (table_bir * 0x4));
  uint32_t bar1 =
      pci_or_pcie_read_dword(bus, slot, func, 0x10 + ((table_bir + 1) * 0x4));

  uint64_t table_base =
      (uint64_t)(((uint64_t)bar1 << 32) | (bar0 & 0xFFFFFFF0)) +
      (msix_cap.table_addr & ~0x7);
  uint64_t table_entry_offset = table_base + (msi_vector * 16);

  uint64_t msg_data = 0;
  uint64_t msg_addr =
      arch_msi_address(&msg_data, irq_vector, get_processor(), 1, 1);

  map_page(table_entry_offset);

  *(volatile uint32_t *)table_entry_offset = (uint32_t)(msg_addr & 0xFFFFFFFC);
  *(volatile uint32_t *)(table_entry_offset + 4) = (uint32_t)(msg_addr >> 32);
  *(volatile uint32_t *)(table_entry_offset + 8) = (uint32_t)msg_data;
  *(volatile uint32_t *)(table_entry_offset + 12) = (uint32_t)(0 & ~1);

  return false;
}

bool enable_msi(uint8_t bus, uint8_t slot, uint8_t func, uint8_t irq_vector) {
  msi_capability msi_cap;

  uint8_t cap_ptr = find_msi_capability(bus, slot, func);
  if (cap_ptr == 0)
    return true;

  msi_cap.msg_control =
      pci_or_pcie_read_dword(bus, slot, func, cap_ptr + 2) & 0xFFFF;

  msi_cap.msg_control |= 0x0001;
  pci_or_pcie_write_dword(bus, slot, func, cap_ptr + 2, msi_cap.msg_control);

  uint64_t msi_data = 0;
  uint64_t msi_addr =
      arch_msi_address(&msi_data, irq_vector, get_processor(), 1, 1);

  if (msi_cap.msg_control & (1 << 7)) {
    pci_or_pcie_write_dword(bus, slot, func, cap_ptr + 0x4,
                            (uint32_t)(msi_addr & 0xFFFFFFFC));
    pci_or_pcie_write_dword(bus, slot, func, cap_ptr + 0x8,
                            (uint32_t)(msi_addr >> 32));
    pci_or_pcie_write_dword(bus, slot, func, cap_ptr + 0xC, (uint32_t)msi_data);
  } else {
    pci_or_pcie_write_dword(bus, slot, func, cap_ptr + 0x4,
                            (uint32_t)(msi_addr & 0xFFFFFFFC));
    pci_or_pcie_write_dword(bus, slot, func, cap_ptr + 0x8, (uint32_t)msi_data);
  }
  return false;
}

uint16_t getVendorID(uint8_t bus, uint8_t device, uint8_t function) {
  uint16_t r0 =
      (uint16_t)(pci_or_pcie_read_dword(bus, device, function, 0) & 0xFFFF);
  return r0;
}

uint16_t getDeviceID(uint8_t bus, uint8_t device, uint8_t function) {
  uint16_t r0 =
      (uint16_t)((pci_or_pcie_read_dword(bus, device, function, 2) >> 16) &
                 0xFFFF);
  return r0;
}

uint16_t getClassId(uint8_t bus, uint8_t device, uint8_t function) {
  uint16_t r0 =
      (uint16_t)(pci_or_pcie_read_dword(bus, device, function, 0xA) & 0xFFFF);
  return (r0 & ~0x00FF) >> 8;
}

uint16_t getSubClassId(uint8_t bus, uint8_t device, uint8_t function) {
  uint16_t r0 =
      (uint16_t)(pci_or_pcie_read_dword(bus, device, function, 0xA) & 0xFFFF);
  return (r0 & ~0xFF00);
}

uint8_t find_pci_legacy_capability(uint8_t bus, uint8_t slot, uint8_t func) {
  uint8_t cap_pointer = pci_read_dword(bus, slot, func, 0x34);
  while (cap_pointer != 0) {
    uint8_t cap_id = (uint8_t)pci_read_dword(bus, slot, func, cap_pointer);
    if (cap_id == 0x7)
      return cap_pointer;
    cap_pointer = pci_read_dword(bus, slot, func, cap_pointer + 1);
  }
  return 0;
}

void pci_probe() {
  SYSLOG(SteveStudiosLegacyPCIDriver, Probe PCI without ECAM...)
  
  for (int bus = 0; bus < 256; bus++) {
    for (int slot = 0; slot < 32; slot++) {
      for (int function = 0; function < 8; function++) {
        uint32_t vendor = getVendorID(bus, slot, function);
        if (vendor == 0xFFFF)
          break;

        uint32_t device = getDeviceID(bus, slot, function);

        pci_device pdev;
        pdev.vendor = vendor;
        pdev.device = device;
        pdev.func = function;
        pdev.bus = bus;
        pdev.slot = slot;
        pdev.type = (find_pci_legacy_capability(bus, slot, function) != 0);

        add_pci_device(pdev);
      }
    }
  }
}