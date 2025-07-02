// nvme.c - NVMe Driver
// Created 2024/05/12 by Stephen Byrne

#include "nvme.h"

#define Q_SIZE 63

bool nvme_ready = false;

uint64_t nvme_base_addr;

nvme_queue admin_submission_queue;
nvme_queue admin_completion_queue;

nvme_queue submission_queue;
nvme_queue completion_queue;

uint32_t submission_queue_tail = 0;
uint32_t completion_queue_head = 0;

uint32_t nvme_cap_strd = 0;
uint32_t nsid = 0;

uint32_t nvme_read_reg(uint32_t offset) {
  volatile uint32_t *nvme_reg = (volatile uint32_t *)(nvme_base_addr + offset);
  map_page((uint64_t)nvme_reg);

  return *nvme_reg;
}

void nvme_write_reg(uint32_t offset, uint32_t value) {
  volatile uint32_t *nvme_reg = (volatile uint32_t *)(nvme_base_addr + offset);
  map_page((uint64_t)nvme_reg);

  *nvme_reg = value;
}

bool create_io_completion_queue(nvme_queue *cq) {
  cq->address = (uint64_t)malloc(PAGE_SIZE);
  if (cq->address == 0)
    return false;
  cq->size = Q_SIZE;

  nvme_command_entry command_entry;
  command_entry.opcode = 0x05;
  command_entry.nsid = 0;
  command_entry.prp1 = cq->address;
  command_entry.prp2 = 0;

  uint16_t size = Q_SIZE;

  uint16_t flags = (1 << 0) | (1 << 1);
  uint16_t cqid = 1;
  uint16_t vec = 1;

  command_entry.command_specific[0] = (size << 16) | cqid;
  command_entry.command_specific[1] = (vec << 16) | flags;

  memcpy((void *)admin_submission_queue.address + sizeof(nvme_command_entry),
         &command_entry, sizeof(nvme_command_entry));

  nvme_write_reg(0x1000, (uint32_t)2);
  return true;
}

bool create_io_submission_queue(nvme_queue *sq) {
  sq->address = (uint64_t)malloc(PAGE_SIZE);
  if (sq->address == 0)
    return false;
  sq->size = Q_SIZE;

  nvme_command_entry command_entry;
  command_entry.opcode = 0x01;
  command_entry.nsid = 0;
  command_entry.prp1 = sq->address;
  command_entry.prp2 = 0;

  uint16_t size = Q_SIZE;
  uint16_t id = 1;

  uint16_t flags = (1 << 0) | (1 << 1);
  uint16_t cid = 1;

  command_entry.command_specific[0] = (size << 16) | id;
  command_entry.command_specific[1] = (cid << 16) | flags;

  memcpy((void *)admin_submission_queue.address +
             (sizeof(nvme_command_entry) * 2),
         &command_entry, sizeof(nvme_command_entry));

  nvme_write_reg(0x1000, (uint32_t)3);
  return true;
}

bool create_admin_submission_queue(nvme_queue *sq) {
  sq->address = (uint64_t)malloc(PAGE_SIZE);
  if (sq->address == 0)
    return false;
  sq->size = Q_SIZE;

  nvme_write_reg(0x28, sq->address);
  return true;
}

bool create_admin_completion_queue(nvme_queue *cq) {
  cq->address = (uint64_t)malloc(PAGE_SIZE);
  if (cq->address == 0)
    return false;
  cq->size = Q_SIZE;

  nvme_write_reg(0x30, cq->address);
  return true;
}

bool nvme_send_command(uint8_t opcode, uint32_t nsid, void *prp1, uint64_t lba,
                       uint16_t num_blocks, nvme_completion *completion) {
  uint64_t sq_entry_addr = submission_queue.address +
                           (submission_queue_tail * sizeof(nvme_command_entry));
  uint64_t cq_entry_addr = completion_queue.address +
                           (completion_queue_head * sizeof(nvme_completion));

  nvme_command_entry command_entry;
  command_entry.opcode = opcode;
  command_entry.nsid = nsid;
  command_entry.prp1 = (uintptr_t)prp1;
  command_entry.prp2 = 0;

  command_entry.command_specific[0] = (uint32_t)lba;
  command_entry.command_specific[1] = (uint32_t)((uint64_t)lba >> 32);
  command_entry.command_specific[2] = (uint16_t)(num_blocks - 1);

  memcpy((void *)sq_entry_addr, &command_entry, sizeof(nvme_command_entry));

  submission_queue_tail++;
  nvme_write_reg(0x1000 + 2 * (4 << nvme_cap_strd), submission_queue_tail);

  if (submission_queue_tail == 64)
    submission_queue_tail = 0;
  
  while (!nvme_ready)
    ;
  nvme_ready = false;
  
  map_page(cq_entry_addr);
  completion = (nvme_completion *)cq_entry_addr;

  completion_queue_head++;
  nvme_write_reg(0x1000 + 3 * (4 << nvme_cap_strd), completion_queue_head);

  if (completion_queue_head == 64)
    completion_queue_head = 0;

  return completion->status != 0;
}

bool nvme_read(uint64_t lba, uint32_t sector_count, void *buffer) {
  nvme_completion *completion = NULL;
  if (nvme_send_command(0x02, nsid, buffer, lba, sector_count, completion) !=
      NVME_SUCCESS)
      return true;

    if (completion->status != NVME_SUCCESS)
      return true;
  return false;
}

bool nvme_write(uint64_t lba, uint32_t sector_count, void *buffer) {
  nvme_completion *completion = NULL;
  if (nvme_send_command(0x01, nsid, buffer, lba, sector_count, completion) !=
      NVME_SUCCESS)
    return true;

  if (completion->status != NVME_SUCCESS)
    return true;
  return false;
}

#define PCI_COMMAND_OFFSET 0x04
#define PCI_COMMAND_IO_ENABLE (1 << 0)
#define PCI_COMMAND_MEMORY_ENABLE (1 << 1)
#define PCI_COMMAND_BUS_MASTER_ENABLE (1 << 2)
#define PCI_COMMAND_INTERRUPT_ENABLE (1 << 10)

bool enable_nvme_features(int bus, int slot, int func) {
  uint32_t command_register =
      pci_or_pcie_read_dword(bus, slot, func, PCI_COMMAND_OFFSET);

  command_register |= PCI_COMMAND_IO_ENABLE;
  command_register |= PCI_COMMAND_MEMORY_ENABLE;
  command_register |= PCI_COMMAND_BUS_MASTER_ENABLE;
  command_register |= PCI_COMMAND_INTERRUPT_ENABLE;

  pci_or_pcie_write_dword(bus, slot, func, PCI_COMMAND_OFFSET, command_register);

  uint32_t new_command_register =
      pci_or_pcie_read_dword(bus, slot, func, PCI_COMMAND_OFFSET);

  if ((new_command_register &
       (PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEMORY_ENABLE |
        PCI_COMMAND_BUS_MASTER_ENABLE | PCI_COMMAND_INTERRUPT_ENABLE)) ==
      (PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEMORY_ENABLE |
       PCI_COMMAND_BUS_MASTER_ENABLE | PCI_COMMAND_INTERRUPT_ENABLE))
    return false;
  else
    return true;
}

#define NVME_CC_REG 0x14
#define NVME_CC_INT_EN_MASK (1 << 0)

bool is_namespace_active(uint32_t identify_data[], uint32_t desired_nsid) {
  for (int i = 0; i < 1024; i++) {
    if (identify_data[i] == desired_nsid)
      return true;
  }
  return false;
}

bool nvme_reload_driver(int port_num) {
  free((void *)submission_queue.address);
  free((void *)completion_queue.address);

  free((void *)admin_submission_queue.address);
  free((void *)admin_completion_queue.address);

  pci_device device = devices[port_num];

  if (get_pci_disk_interface(device.bus, device.slot, device.func) != NVME)
    return true;

  // Get both BARs
  uint32_t bar0 = pci_or_pcie_read_dword(device.bus, device.slot, device.func, 0x10);
  uint32_t bar1 = pci_or_pcie_read_dword(device.bus, device.slot, device.func, 0x14);

  // Combine them and get the base address
  nvme_base_addr = (uint64_t)(((uint64_t)bar1 << 32) | (bar0 & 0xFFFFFFF0));
  map_page(nvme_base_addr);

  nvme_cap_strd = (nvme_base_addr >> 12) & 0xF;

  if (enable_nvme_features(device.bus, device.slot, device.func))
    return true;

  if (enable_msix(device.bus, device.slot, device.func, 1, 48))
    enable_msi(device.bus, device.slot, device.func, 48);

  uint32_t cc_value = nvme_read_reg(NVME_CC_REG);
  cc_value &= ~NVME_CC_INT_EN_MASK;
  nvme_write_reg(NVME_CC_REG, cc_value);

  nvme_write_reg(0x24, (64 << 16) | 64);

  memset(&admin_submission_queue, 0, sizeof(nvme_queue));
  if (!create_admin_submission_queue(&admin_submission_queue)) {
    free((void *)admin_submission_queue.address);
    return true;
  }

  memset(&admin_completion_queue, 0, sizeof(nvme_queue));
  if (!create_admin_completion_queue(&admin_completion_queue)) {
    free((void *)admin_completion_queue.address);
    return true;
  }

  cc_value = nvme_read_reg(NVME_CC_REG);
  cc_value |= NVME_CC_INT_EN_MASK;
  nvme_write_reg(NVME_CC_REG, cc_value);

  uint32_t cc_masked =
      nvme_read_reg(NVME_CC_REG) & ~((0xF << 16) | (0xF << 20));

  uint32_t iosqes = 6 & 0xF;
  uint32_t iocqes = 4 & 0xF;

  uint32_t iosqes_shifted = iosqes << 16;
  uint32_t iocqes_shifted = iocqes << 20;

  uint32_t cc_modified = cc_masked | iosqes_shifted | iocqes_shifted;
  nvme_write_reg(NVME_CC_REG, cc_modified);

  nvme_command_entry feature_entry;
  feature_entry.opcode = 0x09;
  feature_entry.nsid = 0;
  feature_entry.prp1 = 0;
  feature_entry.prp2 = 0;

  uint32_t fid = 0x07;
  uint16_t io_count_comp = 0;
  uint16_t io_count_sub = 0;

  uint32_t feature = ((uint32_t)io_count_comp << 16) | io_count_sub;

  feature_entry.command_specific[0] = fid;
  feature_entry.command_specific[1] = feature;

  memcpy((void *)admin_submission_queue.address, &feature_entry,
         sizeof(nvme_command_entry));
  nvme_write_reg(0x1000, (uint32_t)1);

  if (!create_io_completion_queue(&completion_queue)) {
    free((void *)completion_queue.address);
    return true;
  }

  if (!create_io_submission_queue(&submission_queue)) {
    free((void *)submission_queue.address);
    return true;
  }

  uint32_t data[1024];
  nvme_command_entry command_entry;

  command_entry.opcode = 0x06;
  command_entry.nsid = 0;
  command_entry.prp1 = (uintptr_t)data;
  command_entry.prp2 = 0;

  command_entry.command_specific[0] &= 0xFFFF0000;
  command_entry.command_specific[0] |= 2;

  memcpy((void *)admin_submission_queue.address +
             (sizeof(nvme_command_entry) * 3),
         &command_entry, sizeof(nvme_command_entry));
  nvme_write_reg(0x1000, (uint32_t)4);

  for (int i = 1; i < 1024; i++) {
    if (is_namespace_active(data, i)) {
      nsid = i;
      break;
    }
  }

  return false;
}