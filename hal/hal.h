// hal.h - Hardware Abstraction Layer
// Created 2024/03/29 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "utils/hal_utils.h"

hal_disk_interface get_pci_disk_interface(uint32_t bus, uint32_t slot,
                                          uint32_t func);
hal_driver_response hal_call(hal_driver_func func, hal_driver_args args);

#ifdef __cplusplus
}
#endif