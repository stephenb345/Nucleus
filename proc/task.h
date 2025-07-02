// task.h - Nucleus task utility
// Created 2024/08/22 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    const char* name;
    void* entry;
    uint8_t processor;
    bool vacant;
} task_t;

extern volatile task_t* task;

#ifdef __cplusplus
}
#endif