// handler_keyboardps2.h - PS/2 Keyboard handler
// Created 2023/7/20 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../drivers/ps2/keyboardps2.h"
#include "../utils/cstd/str.h"
#include "../utils/display/io.h"

void handle_keyboard();

#ifdef __cplusplus
}
#endif