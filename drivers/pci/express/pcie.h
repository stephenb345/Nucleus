// pcie.h - PCIe Driver
// Created 2024/04/07 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../mem/paging/paging.h"
#include "../pci.h"

void pcie_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
uint32_t pcie_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

#ifdef __cplusplus
}
#endif