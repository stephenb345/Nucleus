// gpt.h - GPT Parser
// Created 2024/02/14 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "../../hal/hal.h"
#include "../../utils/bytes.h"
#include "io/ahci.h"

typedef struct {
  char boot_indicator;
  char chs_start[32];
  char os_type;
  char chs_end[32];
  uint32_t lba_start;
  uint32_t lba_end;
} __attribute__((packed)) gpt_mb_record;

typedef struct {
  uint64_t sig;
  uint32_t gpt_rv;
  uint32_t header_size;
  uint32_t crc_checksum;
  uint32_t __reserved;
  uint64_t parent_lba;
  uint64_t alt_gpt_lba;
  uint64_t first_gpt_block;
  uint64_t last_gpt_block;
  __uint128_t disk_guid;
  uint64_t start_guid_lba_pe;
  uint32_t partition_entries;
  uint32_t partition_entry_table_size;
  uint32_t crc_pe_array;
} __attribute__((packed)) gpt_pt_header;

typedef struct {
  __uint128_t pt_guid;
  __uint128_t unique_partition_guid;
  uint64_t lba_start;
  uint64_t lba_end;
  uint64_t attributes;
  char partname[72];
} __attribute__((packed)) gpt_pt_entry;

extern bool nvme_reload_driver(int port_num);

extern bool nvme_read(uint64_t lba, uint32_t sector_count,
                      void *buffer);
extern bool nvme_write(uint64_t lba, uint32_t sector_count,
                       void *buffer);

extern gpt_pt_entry partitions[128];
void gpt_init(int);

uint64_t get_gpt_start(uint64_t);
uint64_t get_gpt_end(uint64_t);

#ifdef __cplusplus
}
#endif