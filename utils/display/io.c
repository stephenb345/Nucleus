// io.c - Terminal and input/output utilities
// Created 2023/7/11 by Stephen Byrne

#include "io.h"

bool initialized = false;
struct flanterm_context *ft_ctx;

static atomic_flag mem_lock_io = ATOMIC_FLAG_INIT;

void io_lock() {
  mem_lock_acquire(&mem_lock_io);
}

void io_unlock() {
  mem_lock_release(&mem_lock_io);
}

// Get the flanterm context; if it does not exist, initialize it
struct flanterm_context *get_ft_ctx() {
  if (!initialized) {
    ft_ctx = flanterm_fb_simple_init(
        (uint32_t *)boot_parameters->framebuffer, boot_parameters->width,
        boot_parameters->height, boot_parameters->pitch);
    initialized = true;
  }
  return ft_ctx;
}

// Clear the terminal by reinitializing the flanterm context
void clear_term() {
  ft_ctx = flanterm_fb_simple_init(
      (uint32_t *)boot_parameters->framebuffer, boot_parameters->width,
      boot_parameters->height, boot_parameters->pitch);
}

// Write a string to video memory, printing a newline afterwards
void println(const char *str) {
  io_lock();

  struct flanterm_context *context = get_ft_ctx();

  flanterm_write(context, str, strlen(str));
  flanterm_write(context, "\n", strlen("\n"));

  io_unlock();
}

// Write a string to video memory
void print(const char *str) {
  io_lock();

  struct flanterm_context *context = get_ft_ctx();
  flanterm_write(context, str, strlen(str));

  io_unlock();
}

void syslog(const char *str) { println(str); }

// Write a string to video memory, printing a newline afterwards
void println_int(__int128_t n) {
  struct flanterm_context *context = get_ft_ctx();
  
  if (n == 0) {
    flanterm_write(context, "0\n", 2);
    return;
  }
  char int_str[33];
  int len = itoa(n, int_str, 10);

  flanterm_write(context, int_str, len);
  flanterm_write(context, "\n", 1);

}

// Write a string to video memory
void print_int(__int128_t n) {
  io_lock();

  struct flanterm_context *context = get_ft_ctx();

  if (n == 0) {
    flanterm_write(context, "0", 1);
    return;
  }
  char int_str[33];
  int len = itoa(n, int_str, 10);

  flanterm_write(context, int_str, len);

  io_unlock();
}