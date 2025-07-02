// hal_utils.c - Hardware Abstraction Functions
// Created 2024/03/29 by Stephen Byrne

#include "hal_utils.h"

extern HBA_MEM *mem_hba;

bool hal_disk_read(hal_disk_interface interface, uint64_t start_sector,
                   uint32_t len, void *buf) {
  map_page((uint64_t)buf);

  switch (interface) {
  case AHCI_SATA:
    if (ahci_probe_port(mem_hba))
      return false;
    return ahci_read(&mem_hba->ports[0], start_sector, 0, len, buf);
  case NVME:
    return !nvme_read(start_sector, len, buf);
  case ATAPI:
    return !atapi_read(0, start_sector, len, buf);
  case IDE:
    return !ide_read(0, len, start_sector, 0x10, buf);
  case NIL:
    break;
  }
  return false;
}

bool hal_disk_write(hal_disk_interface interface, uint64_t start_sector,
                    uint32_t len, void *buf) {
  map_page((uint64_t)buf);

  switch (interface) {
  case AHCI_SATA:
    if (ahci_probe_port(mem_hba))
      return false;
    return ahci_write(&mem_hba->ports[0], start_sector, 0, len, buf);
  case NVME:
    return !nvme_write(start_sector, len, buf);
  case ATAPI:
    return !atapi_write(0, start_sector, len, buf);
  case IDE:
    return !ide_write(0, len, start_sector, 0x10, buf);
  case NIL:
    break;
  }
  return false;
}