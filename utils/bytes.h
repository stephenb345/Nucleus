// bytes.h - Utilities for working with bytes
// Created 2024/03/14 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint32_t get_high_bytes_32(uint64_t x);
uint32_t get_low_bytes_32(uint64_t x);

uint16_t get_high_bytes_16(uint32_t x);
uint16_t get_low_bytes_16(uint32_t x);

#ifdef __cplusplus
extern "C"
}
#endif