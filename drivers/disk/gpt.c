// gpt.c - GPT Parser
// Created 2024/02/14 by Stephen Byrne

#include "gpt.h"

HBA_MEM *mem_hba;

gpt_pt_entry partitions[128];
gpt_pt_header *header;

extern pci_device devices[8192];

extern uint32_t pcie_read(uint8_t bus, uint8_t slot, uint8_t func,
                          uint8_t offset);

uint64_t get_gpt_start(uint64_t entry) {
  return (partitions[entry].lba_start != 0xFF &&
          partitions[entry].lba_end != 0xFF)
             ? partitions[entry].lba_start
             : 0xFF;
}

uint64_t get_gpt_end(uint64_t entry) {
  return (partitions[entry].lba_start != 0xFF &&
          partitions[entry].lba_end != 0xFF)
             ? partitions[entry].lba_end
             : 0xFF;
}

void clear_partitions_table() {
  for (int i = 0; i < 128; i++) {
    partitions[i].lba_start = 0xFF;
    partitions[i].lba_end = 0xFF;
  }
}

void gpt_init(int port_num) {
  uint32_t bus = devices[port_num].bus;
  uint32_t slot = devices[port_num].slot;
  uint32_t func = devices[port_num].func;

  if (get_pci_disk_interface(bus, slot, func) == NIL)
    return;

  clear_partitions_table();

  if (get_pci_disk_interface(bus, slot, func) == AHCI_SATA) {
    mem_hba = (HBA_MEM *)(uintptr_t)pci_or_pcie_read_dword(bus, slot, func, 0x24);
    map_page((uint64_t)mem_hba);
  }

  if (get_pci_disk_interface(bus, slot, func) == NVME)
    nvme_reload_driver(port_num);

  char partitions_buf[SECTOR_SIZE];
  hal_disk_read(get_pci_disk_interface(bus, slot, func), 1, 1, partitions_buf);

  map_page((uint64_t)partitions_buf);
  header = (gpt_pt_header *)partitions_buf;

  int entries_buf_size = SECTOR_SIZE;
  if ((header->partition_entry_table_size * header->partition_entries) > 0)
    entries_buf_size =
        ((header->partition_entry_table_size * header->partition_entries) /
         SECTOR_SIZE) *
        SECTOR_SIZE;

  char entries_buf[entries_buf_size];
  hal_disk_read(
      get_pci_disk_interface(bus, slot, func), 2,
      ((header->partition_entry_table_size * header->partition_entries) +
       SECTOR_SIZE) /
          SECTOR_SIZE,
      entries_buf);

  gpt_pt_entry partitions_with_unused[header->partition_entries];
  int unused_space = 0;

  for (int i = 0; i < header->partition_entries; i++) {
    char single_entry[header->partition_entry_table_size];
    int n = 0;
    for (int j = header->partition_entry_table_size * i;
         j < ((header->partition_entry_table_size * i) +
              header->partition_entry_table_size);
         j++) {
      single_entry[n] = (char)entries_buf[j];
      n++;
    }
    partitions_with_unused[i] = *(gpt_pt_entry *)single_entry;
    if ((int)(partitions_with_unused[i].pt_guid) == 0)
      unused_space += (int)header->partition_entry_table_size;
  }

  int j = 0;
  for (int i = 0; i < header->partition_entries; i++) {
    if ((int)(partitions_with_unused[i].pt_guid) != 0) {
      partitions[j] = partitions_with_unused[i];
      j++;
    }
  }
}