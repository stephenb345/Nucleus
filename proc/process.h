// process.h - UNIX Process Driver
// Created 2024/05/11 by Stephen Byrne

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "elfloader.h"
#include "task.h"

#define MAX_RESPONSE_TIME 125000000

typedef uint32_t pid_t;
typedef uint64_t clock_t;

typedef struct {
  bool is_nil;
  uint32_t max_time;
  uint8_t state;
  uint8_t priority;
  pid_t pid;
  uint64_t *stack_top;
  uint64_t *cr3;
  char *task_name;
  clock_t time_consumed;
  clock_t last_time_consumed;
  void* entry;
} tcb_t;

void create_thread(char *name, void *entry);

void schedule();
void update_thread_time();

extern void start_task_entry(void* entry);
extern void switch_to_task(tcb_t *next_thread);

#ifdef __cplusplus
}
#endif