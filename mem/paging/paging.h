// paging.h - Paging Manager
// Created 2024/04/08 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../../include/global.h"
#include "../../utils/cstd/mem.h"
#include "../../utils/cstd/memalloc.h"
#include "../../cpu/interrupts/idt.h"

void *phys_to_virt(uint64_t phys_addr);

void map_page(uint64_t i);
void setup_paging();

#ifdef __cplusplus
}
#endif