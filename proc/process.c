// process.c - UNIX Process Driver
// Created 2024/05/11 by Stephen Byrne

#include "process.h"

pid_t last_pid = 0;

uint32_t thread_count = 0;
uint32_t thread_bitmap_size = sizeof(tcb_t *);

tcb_t *all_threads;
tcb_t *current_thread = NULL;

uint64_t get_tsc() {
  uint32_t lo, hi;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

uint8_t original_state = 0;

static atomic_flag mem_lock_sched = ATOMIC_FLAG_INIT;

void sched_lock() { mem_lock_acquire(&mem_lock_sched); }
void sched_unlock() { mem_lock_release(&mem_lock_sched); }

void scheduler_lock() {
  sched_lock();
  original_state = current_thread->state;
  current_thread->state = 0x3;
}

void scheduler_unlock() {
  sched_unlock();
  current_thread->state = original_state;
}

#define T_LOCK scheduler_lock();
#define T_UNLOCK scheduler_unlock();

void schedule() {
  uint8_t priority = 0;
  int least_time_consumed = 0;

  for (int i = 0; i < thread_bitmap_size; i++) {
    if (all_threads[i].is_nil || all_threads[i].state == 0x3)
      continue;
    switch (all_threads[i].priority) {
    case 0:
    case 1:
    case 2:
    case 3:
      if (all_threads[i].time_consumed > least_time_consumed)
        least_time_consumed = all_threads[i].time_consumed;
      break;
    default:
      break;
    }
  }
  if (least_time_consumed < current_thread->time_consumed) {
    T_LOCK

    for (int i = 0; i < thread_bitmap_size; i++) {
      if (all_threads[i].is_nil || all_threads[i].state == 0x3)
        continue;
      if (all_threads[i].time_consumed == least_time_consumed &&
          all_threads[i].priority == priority)
        current_thread = &all_threads[i];
    }

    T_UNLOCK
  }
  current_thread->max_time =
      (MAX_RESPONSE_TIME / (thread_count == 0 ? 1 : thread_count));
  switch_to_task(current_thread);
  if (current_thread->state == 0x0) {
    current_thread->state = 0x1;
    start_task_entry(
        (*((void *(*__attribute__((sysv_abi)))())(current_thread->entry))));
  }
}

void update_thread_time() {
  if (current_thread == NULL)
    return;

  uint64_t current_count = get_tsc();
  uint64_t elapsed = current_thread->last_time_consumed - current_count;
  current_thread->last_time_consumed = current_count;
  current_thread->time_consumed += elapsed;

  if (current_thread->time_consumed < current_thread->max_time)
    return;
  else
    schedule();
}

void map_thread_page(uint64_t i, uint64_t *pml4) {
  uint64_t pml4_index = (i >> 39) & 0x1FF;
  uint64_t pdpt_index = (i >> 30) & 0x1FF;
  uint64_t pd_index = (i >> 21) & 0x1FF;
  uint64_t pt_index = (i >> 12) & 0x1FF;

  uint64_t *pdpt;
  uint64_t *pd;
  uint64_t *pt;

  if (pml4[pml4_index] == 0) {
    T_LOCK

    pdpt = (uint64_t *)alloc_page(1);
    memset(pdpt, 0, PAGE_SIZE);
    pml4[pml4_index] = (uint64_t)pdpt | 0b111;

    T_UNLOCK
  }
  pdpt = (uint64_t *)(pml4[pml4_index] & 0x000FFFFFFFFFF000);

  if (pdpt[pdpt_index] == 0) {
    T_LOCK

    pd = (uint64_t *)alloc_page(1);
    memset(pd, 0, PAGE_SIZE);
    pdpt[pdpt_index] = (uint64_t)pd | 0b111;

    T_UNLOCK
  }
  pd = (uint64_t *)(pdpt[pdpt_index] & 0x000FFFFFFFFFF000);

  if (pd[pd_index] == 0) {
    T_LOCK

    pt = (uint64_t *)alloc_page(1);
    memset(pt, 0, PAGE_SIZE);
    pd[pd_index] = (uint64_t)pt | 0b111;

    T_UNLOCK
  }

  pt = (uint64_t *)(pd[pd_index] & 0x000FFFFFFFFFF000);
  pt[pt_index] = i | 0b111;
}

void delete_thread(pid_t pid) {
  for (int i = 0; i < thread_bitmap_size; i++) {
    if (all_threads[i].is_nil)
      continue;
    if (all_threads[i].pid == pid) {
      last_pid = pid;
      T_LOCK

      tcb_t *block_to_del = &all_threads[i];

      free_page(block_to_del->cr3,
                (((sizeof(uint64_t) * PAGE_SIZE) / PAGE_SIZE) * PAGE_SIZE) /
                    PAGE_SIZE);

      free(block_to_del->stack_top);

      all_threads[i].is_nil = true;
      free(&all_threads[i - 0x100000]);

      thread_bitmap_size = thread_count * sizeof(tcb_t *);
      all_threads = realloc(all_threads, thread_bitmap_size);

      thread_count--;

      T_UNLOCK

      schedule();
      break;
    }
  }
}

bool init = true;
uint8_t current_proc = 0;

void create_thread(char *name, void *entry) {
  if (current_proc != 0) {
    if (task != NULL)
      free((void*)task);
    task = NULL;

    if (current_proc == boot_parameters->lapic_ids_size) {
      current_proc = 0;
    } else {
      task = malloc(sizeof(task_t));
      task->name = name;
      task->entry = entry;
      task->processor = current_proc;
      task->vacant = true;
      
      current_proc++;

      return;
    }
  }
  tcb_t *new_block = malloc(sizeof(tcb_t));
  memset(new_block, 0, sizeof(tcb_t));

  if (init) {
    all_threads = malloc(sizeof(tcb_t *));
    init = false;
  }

  uint32_t index = thread_bitmap_size;
  for (int i = 0; i < thread_bitmap_size; i++) {
    if (all_threads[i].is_nil) {
      index = i;
      break;
    }
  }

  if (index == thread_bitmap_size && thread_bitmap_size > sizeof(tcb_t *)) {
    thread_bitmap_size += sizeof(tcb_t *);
    all_threads = realloc(all_threads, thread_bitmap_size);
  };

  T_LOCK

  uint64_t *pml4 = (uint64_t *)alloc_page(1);
  memset(pml4, 0, PAGE_SIZE);

  for (uint32_t i = PAGE_SIZE;
       i <
       (((sizeof(uint64_t) * PAGE_SIZE) / PAGE_SIZE) * PAGE_SIZE) + PAGE_SIZE;
       i += PAGE_SIZE) {
    map_thread_page(i, pml4);
  }

  T_UNLOCK

  new_block->task_name = name;
  new_block->state = 0x0;
  new_block->pid = last_pid + 1;
  new_block->max_time =
      (MAX_RESPONSE_TIME / (thread_count == 0 ? 1 : thread_count));
  new_block->cr3 = pml4;
  new_block->time_consumed = 0;
  new_block->stack_top = (uint64_t *)(uintptr_t)(malloc(0x100000) + 0x100000);
  new_block->priority = 2;
  new_block->entry = entry;

  last_pid = new_block->pid;
  for (int i = 0; i < thread_bitmap_size; i++) {
    if (all_threads[i].is_nil)
      continue;
    if (all_threads[i].pid > last_pid)
      last_pid = all_threads[i].pid;
  }

  all_threads[index] = *new_block;

  thread_count++;
  current_proc++;

  schedule();
}