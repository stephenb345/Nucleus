// handler_syscall.c - System call handler
// Created 2023/11/18 by Stephen Byrne

#include "handler_syscall.h"

extern tcb_t *current_thread;

// Called from (cpu/interrupts/asm/irq_helper.asm)
void handle_syscall(int rax, void *arg0, void *arg1, void *arg2, void *arg3,
                   void *arg4, void *arg5) {
  void *retval = NULL;

  switch (rax) {
  // UNIX 'read'
  case 0:
    retval = (void *)read((intptr_t)arg0, arg1, (intptr_t)arg2);
    if ((int)(intptr_t)retval != 0)
      retval = (void *)(intptr_t)-1;
    break;
  // UNIX 'write'
  case 1:
    retval = (void *)write((intptr_t)arg0, arg1, (intptr_t)arg2);
    if ((int)(intptr_t)retval != 0)
      retval = (void *)(intptr_t)-1;
    break;
    // UNIX 'open'
  case 2:
    retval = (void *)(intptr_t)open(arg0, (intptr_t)arg1, arg2);
    if ((int)(intptr_t)retval != 0)
      retval = (void *)(intptr_t)-1;
    break;
  // UNIX 'close'
  case 3:
    retval = (void *)(intptr_t)close((intptr_t)arg0);
    if ((int)(intptr_t)retval != 0)
      retval = (void *)(intptr_t)-1;
    break;
  // POSIX 'stat'
  case 4:
    retval = (void *)(intptr_t)stat(arg0, arg1);
    if ((int)(intptr_t)retval != 0)
      retval = (void *)(intptr_t)-1;
    break;
  // POSIX 'fstat'
  case 5:
    if (!descriptor_exists((int)(intptr_t)arg0)) {
      retval = (void *)(intptr_t)-1;
      if ((int)(intptr_t)retval != 0)
        retval = (void *)(intptr_t)-1;
      break;
    }
    const char *resolved_path = get_descriptor((int)(intptr_t)arg0)->filename;
    retval = (void *)(intptr_t)stat(resolved_path, arg1);
    if ((int)(intptr_t)retval != 0)
      retval = (void *)(intptr_t)-1;
    break;
  // POSIX 'lstat'
  case 6:
    retval = (void *)(intptr_t)lstat(arg0, arg1);
    if ((int)(intptr_t)retval != 0)
      retval = (void *)(intptr_t)-1;
    break;
  // 'exit'
  case 60:
    if (current_thread == NULL)
      break;

    free(current_thread->stack_top);
    free(current_thread->cr3);
    free(current_thread->task_name);
    free(current_thread->entry);

    free(current_thread);
    current_thread = NULL;

    schedule();
    break;
  }
  if (retval != NULL)
    send_syscall_return_val(retval);
  if (current_thread->time_consumed > current_thread->max_time)
    schedule();
}