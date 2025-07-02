// interrupt.h - Interrupt handler
// Created 2023/7/21 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../../drivers/ps2/keyboardps2.h"
#include "../../handlers/handler_keyboardps2.h"
#include "../../handlers/handler_syscall.h"
#include "../../proc/process.h"

#ifdef __cplusplus
}
#endif