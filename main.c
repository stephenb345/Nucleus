// main.c - File containing the kernel entry point
// Created 2023/7/10 by Stephen Byrne

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "utils/display/io.h"

#include "drivers/disk/gpt.h"
#include "drivers/disk/io/nvme.h"
#include "drivers/filesystems/ssfs.h"
#include "drivers/pci/express/pcie.h"
#include "drivers/pci/pci.h"
#include "drivers/serial.h"

#include "handlers/handler_keyboardps2.h"

#include "cpu/gdt/gdt.h"
#include "cpu/interrupts/idt.h"
#include "cpu/smp/smp.h"

#include "drivers/vfs/vfs.h"
#include "mem/paging/paging.h"
#include "utils/cstd/memalloc.h"

#include "proc/process.h"

#include "posix/close.h"
#include "posix/open.h"
#include "posix/read.h"

#include "lib/miniz/miniz.h"

volatile char *tmpfs_device_file;

uint64_t tmpfs_size = 100000;
bool tmpfs = true;

void *__mz_realloc(void *unused1, void *address, size_t unused2, size_t size) {
  return realloc(address, size);
}

void *__mz_malloc(void *unused1, size_t unused2, size_t size) {
  return malloc(size);
}

void __mz_free(void *unused1, void *address) { free(address); }

void extract_initramfs(const char *zip_data, size_t zip_size,
                       char parent_dir[]) {
  mz_zip_archive zip_archive;
  zip_archive.m_pAlloc = __mz_malloc;
  zip_archive.m_pRealloc = __mz_realloc;
  zip_archive.m_pFree = __mz_free;

  if (!mz_zip_reader_init_mem(&zip_archive, zip_data, zip_size, 0))
    return;

  int num_files = mz_zip_reader_get_num_files(&zip_archive);

  for (int i = 0; i < num_files; i++) {
    char filename[256];
    mz_zip_reader_get_filename(&zip_archive, i, filename, sizeof(filename));

    size_t file_size = 0;
    char *file_data = mz_zip_reader_extract_to_heap(&zip_archive, i,
                                                         &file_size, 0);

    if (file_data == NULL)
      return;

    bool is_folder = false;
    size_t name_len = 0;
    for (int i = 0; i < 256; i++) {
      if (filename[i] == 0) {
        if (filename[i - 4] == '.' && filename[i - 3] == 'z' &&
            filename[i - 2] == 'i' && filename[i - 1] == 'p') {
          name_len = i - 4;
          is_folder = true;
        }
        break;
      }
    }

    if (is_folder) {
      char temp_filename[(int)name_len];
      for (int i = 0; i < name_len; i++) {
        temp_filename[i] = filename[i];
      }
      if (parent_dir != 0) {
        char *name = parent_dir;
        strcat(name, "/");
        strcat(name, temp_filename);

        vmkdir(name);
      } else
        vmkdir(temp_filename);
      extract_initramfs(file_data, file_size, temp_filename);
    } else {
      if (parent_dir == 0)
        vmkfile(filename, file_data);
      else {
        char *name = parent_dir;
        strcat(name, "/");
        strcat(name, filename);

        vmkfile(name, file_data);
      }
    }
    mz_zip_reader_end(&zip_archive);
  }
}

// Disable interrupts and halt
void kernel_hang() {
  for (;;)
    asm("cli; hlt");
}

NUCLEUS_PARAMETERS *boot_parameters;
bool using_custom_pages = false;

// Entry point - this is where the kernel starts
void main(NUCLEUS_PARAMETERS *parameters) {
  asm volatile("mov %0, %%rsp" ::"r"(parameters->stack_end));
  boot_parameters = parameters;

  // Initialize GDT
  SYSLOG(nucleus_kernel, Load GDT...)

  gdt_init();

  SYSLOG(nucleus_kernel, done)

  // Initialize IDT and interrupts
  SYSLOG(nucleus_kernel, Load IDT...)

  idt_init();

  SYSLOG(nucleus_kernel, done)

  // Initialize Serial ports
  init_serial();

  SYSLOG(nucleus_kernel, Setup allocator...)

  // Initialize Allocator
  init_mem();

  // Setup paging
  setup_paging();

  using_custom_pages = true;

  asm volatile("sti");

  SYSLOG(nucleus_kernel, Setup PCI / PCIe drivers...)

  // Initialize PCI and PCIe drivers
  pci_probe();
  pcie_probe();

  SYSLOG(nucleus_kernel, done)
  SYSLOG(nucleus_kernel, Setting up initramfs...)

  tmpfs_device_file = malloc(PAGE_SIZE);
  memset((char*)tmpfs_device_file, 0, PAGE_SIZE);

  for (uint64_t i = PAGE_SIZE; i < (tmpfs_size / PAGE_SIZE) * PAGE_SIZE; i += PAGE_SIZE) {
    char* device_file_part = malloc(PAGE_SIZE);
    memset(device_file_part, 0, PAGE_SIZE);
  }

  ssfs_reload_driver("/dev/tmpfs");

  SYSLOG(nucleus_kernel, done)
  SYSLOG(nucleus_kernel, Copying initramfs to RAM...)

  extract_initramfs(boot_parameters->initramfs, boot_parameters->initramfs_size,
                    0);

  SYSLOG(nucleus_kernel, done)

  // Startup APs
  start_smp();

  extern volatile uint8_t active_aps;
  while (active_aps != boot_parameters->lapic_ids_size - 1)
    asm volatile("pause" : : : "memory");

  SYSLOG(nucleus_kernel, All processors are active)
  SYSLOG(nucleus_kernel, Starting 'init' thread...)

  print("Active processors/cores: ");
  print_int(active_aps);

  for (;;)
    asm("hlt");

  /* Run init thread
  const char *init = "/usr/sbin/init";
  create_thread("init", elf_load_file(vrdfile((char *)init)));
  */
 
  // Make sure the system doesn't exit for no reason
  for (;;)
    asm("hlt");
}
