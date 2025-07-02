// smp.h - SMP Support
// Created 2024/7/22 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "../apic.h"
#include "../pit.h"
#include "../../include/global.h"
#include "../../proc/task.h"
#include "../../proc/process.h"

void multiproc_main();
void start_smp();

#ifdef __cplusplus
}
#endif