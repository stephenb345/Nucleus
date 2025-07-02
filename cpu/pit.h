// pit.h - x86 PIT Driver
// Created 2024/05/12 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/ports.h"
#include "../utils/display/io.h"

#define PIT_CHANNEL_0_DATA_PORT 0x40
#define PIT_COMMAND_PORT 0x43

#define PIT_MODE 0x36
#define PIT_DIV_LOWBYTE 0xE9
#define PIT_DIV_HIGHBYTE 0x04

void reload_pit(uint16_t frequency);
void msleep(uint32_t milliseconds);
void usleep_hundreds(uint32_t hundreds_microseconds);

#ifdef __cplusplus
}
#endif