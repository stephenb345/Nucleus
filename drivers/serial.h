// serial.h - Serial port initialization
// Created 2024/03/04 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../utils/ports.h"
#include "../utils/display/io.h"
#include "../include/global.h"

int init_serial();

#ifdef __cplusplus
}
#endif