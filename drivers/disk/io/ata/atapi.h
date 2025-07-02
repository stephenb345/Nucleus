// atapi.h - Legacy ATAPI Driver
// Created 2024/05/15 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "../../../../utils/ports.h"
#include "../../../../cpu/pit.h"

#define DATA 0
#define ERROR_R 1
#define SECTOR_COUNT 2
#define LBA_LOW 3
#define LBA_MID 4
#define LBA_HIGH 5
#define DRIVE_SELECT 6
#define COMMAND_REGISTER 7
 
#define CONTROL 0x206
 
#define ALTERNATE_STATUS 0

bool atapi_read(uint16_t port, uint32_t lba, uint32_t sectors,
                void *buffer);
bool atapi_write(uint16_t port, uint32_t lba, uint32_t sectors,
                void *buffer);
                
#ifdef __cplusplus
}
#endif