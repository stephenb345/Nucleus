// hal_utils.h - Hardware Abstraction Functions
// Created 2024/03/29 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "../../drivers/disk/io/ahci.h"
#include "../../drivers/disk/io/ata/atapi.h"
#include "../../drivers/disk/io/ata/ide.h"

typedef union {
  bool b;
  int i;
  unsigned int ui;
  char c;
  unsigned char uc;
  char *p_char;
  unsigned char *p_uchar;
} hal_driver_response;

typedef struct {
  HBA_PORT *port;
  uint32_t startl;
  uint32_t starth;
  uint32_t count;
  char *buf;
} hal_driver_args;

typedef enum { HAL_AHCI_READ = 0, HAL_AHCI_WRITE = 1 } hal_driver_func;

typedef enum {
  AHCI_SATA = 0,
  NVME = 1,
  ATAPI = 2,
  IDE = 3,
  VIRTIO = 4,
  NIL = 5
} hal_disk_interface;

extern bool nvme_read(uint64_t lba, uint32_t sector_count,
                      void *buffer);
extern bool nvme_write(uint64_t lba, uint32_t sector_count,
                       void *buffer);

bool hal_disk_read(hal_disk_interface interface, uint64_t start_sector,
                   uint32_t len, void *buf);
bool hal_disk_write(hal_disk_interface interface, uint64_t start_sector,
                    uint32_t len, void *buf);

#ifdef __cplusplus
}
#endif