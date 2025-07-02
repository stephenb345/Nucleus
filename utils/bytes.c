// bytes.c - Utilities for working with bytes
// Created 2024/03/14 by Stephen Byrne

#include "bytes.h"

uint32_t get_high_bytes_32(uint64_t x) { return (x >> 32) & 0xFFFF; }
uint32_t get_low_bytes_32(uint64_t x) { return x & 0xFFFF; }

uint16_t get_high_bytes_16(uint32_t x) { return (x >> 16) & 0xFFFF; }
uint16_t get_low_bytes_16(uint32_t x) { return x & 0xFFFF; }
