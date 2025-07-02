// nvme.h - NVMe Driver
// Created 2024/05/12 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "../../../hal/hal.h"
#include "../../../mem/allocator.h"
#include "../../../mem/paging/paging.h"
#include "../../pci/express/pcie.h"
#include "../../pci/pci.h"

typedef struct {
  uint8_t opcode : 8;
  uint8_t fuse : 2;
  uint8_t reserved1 : 4;
  uint8_t prp_sgl : 2;
  uint16_t cid : 16;
  uint32_t nsid;
  uint32_t reserved2[2];
  uint64_t metadata_ptr;
  uint64_t prp1;
  uint64_t prp2;
  uint32_t command_specific[6];
} nvme_command_entry;

#define NVME_CAP_REG 0x0
#define NVME_CMD_REG 0x10
#define NVME_STS_REG 0x18
#define NVME_INT_REG 0x20
#define NVME_AQA_REG 0x24
#define NVME_ASQ_REG 0x28
#define NVME_ACQ_REG 0x30

#define NVME_CC_REG_OFFSET 0x14
#define NVME_CC_ENABLE_BIT (0 << 0)

#define NVME_STS_RDY 0x1
#define NVME_SUCCESS 0

#define NVME_INTMS_REG 0x0C
#define NVME_INTMC_REG 0x10

typedef struct {
  uint64_t address;
  uint64_t size;
} nvme_queue;

typedef struct {
  uint32_t cmd_specific;
  uint32_t reserved;
  uint16_t sq_head;
  uint16_t sq_id;
  uint16_t cid;
  uint8_t phase : 1;
  uint16_t status;
} __attribute__((packed)) nvme_completion;

extern pci_device devices[8192];

bool nvme_read(uint64_t lba, uint32_t sector_count, void *buffer);
bool nvme_write(uint64_t lba, uint32_t sector_count, void *buffer);

bool nvme_reload_driver(int port_num);

#ifdef __cplusplus
}
#endif