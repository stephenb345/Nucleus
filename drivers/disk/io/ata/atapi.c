// atapi.c - Legacy ATAPI Driver
// Created 2024/05/15 by Stephen Byrne

#include "atapi.h"

#define ATA_TIMEOUT 10000

void insw(uint16_t __port, void *__buf, unsigned long __n) {
  asm volatile("cld; rep; insw" : "+D"(__buf), "+c"(__n) : "d"(__port));
}

void outsw(uint16_t __port, const void *__buf, unsigned long __n) {
  asm volatile("cld; rep; outsw" : "+S"(__buf), "+c"(__n) : "d"(__port));
}

static void ata_io_wait(const uint8_t p) {
  inb(p + CONTROL + ALTERNATE_STATUS);
  inb(p + CONTROL + ALTERNATE_STATUS);
  inb(p + CONTROL + ALTERNATE_STATUS);
  inb(p + CONTROL + ALTERNATE_STATUS);
}

bool atapi_write(uint16_t port, uint32_t lba, uint32_t sectors, void *buffer) {
  volatile uint8_t write_cmd[12] = {0xAA,
                                    0,
                                    (lba >> 0x18) & 0xFF,
                                    (lba >> 0x10) & 0xFF,
                                    (lba >> 0x08) & 0xFF,
                                    (lba >> 0x00) & 0xFF,
                                    (sectors >> 0x18) & 0xFF,
                                    (sectors >> 0x10) & 0xFF,
                                    (sectors >> 0x08) & 0xFF,
                                    (sectors >> 0x00) & 0xFF,
                                    0,
                                    0};

  outb(port + DRIVE_SELECT, 0xA0 & (1 << 4));
  ata_io_wait(port);
  
  outb(port + ERROR_R, 0x00);
  outb(port + LBA_MID, 2048 & 0xFF);
  outb(port + LBA_HIGH, 2048 >> 8);
  outb(port + COMMAND_REGISTER, 0xA0);
  ata_io_wait(port);

  int timeout = 0;
  while (true) {
    timeout++;
    if (timeout >= ATA_TIMEOUT)
      return true;
    uint8_t status = inb(port + COMMAND_REGISTER);
    if ((status & 0x01) == 1)
      return true;
    if (!(status & 0x80) && (status & 0x08))
      break;
    ata_io_wait(port);
  }

  timeout = 0;
  outsw(port + DATA, (uint16_t *)write_cmd, 6);
  for (uint32_t i = 0; i < sectors; i++) {
    while (true) {
      timeout++;
      if (timeout >= ATA_TIMEOUT)
        return true;
      uint8_t status = inb(port + COMMAND_REGISTER);
      if (status & 0x01)
        return true;
      if (!(status & 0x80) && (status & 0x08))
        break;
    }

    int size = inb(port + LBA_HIGH) << 8 | inb(port + LBA_MID);
    outsw(port + DATA, (uint16_t *)((uint8_t *)buffer + i * 0x800), size / 2);
  }
  return false;
}

bool atapi_read(uint16_t port, uint32_t lba, uint32_t sectors, void *buffer) {
  volatile uint8_t read_cmd[12] = {0xA8,
                                   0,
                                   (lba >> 0x18) & 0xFF,
                                   (lba >> 0x10) & 0xFF,
                                   (lba >> 0x08) & 0xFF,
                                   (lba >> 0x00) & 0xFF,
                                   (sectors >> 0x18) & 0xFF,
                                   (sectors >> 0x10) & 0xFF,
                                   (sectors >> 0x08) & 0xFF,
                                   (sectors >> 0x00) & 0xFF,
                                   0,
                                   0};

  outb(port + DRIVE_SELECT, 0xA0 & (1 << 4));
  ata_io_wait(port);

  outb(port + ERROR_R, 0x00);
  outb(port + LBA_MID, 2048 & 0xFF);
  outb(port + LBA_HIGH, 2048 >> 8);
  outb(port + COMMAND_REGISTER, 0xA0);
  ata_io_wait(port);

  int timeout = 0;
  while (true) {
    timeout++;
    if (timeout >= ATA_TIMEOUT)
      return true;
    uint8_t status = inb(port + COMMAND_REGISTER);
    if ((status & 0x01) == 1)
      return true;
    if (!(status & 0x80) && (status & 0x08))
      break;
    ata_io_wait(port);
  }

  outsw(port + DATA, (uint16_t *)read_cmd, 6);
  for (uint32_t i = 0; i < sectors; i++) {
    while (true) {
      timeout++;
      if (timeout >= ATA_TIMEOUT)
        return true;
      uint8_t status = inb(port + COMMAND_REGISTER);
      if (status & 0x01)
        return true;
      if (!(status & 0x80) && (status & 0x08))
        break;
    }

    int size = inb(port + LBA_HIGH) << 8 | inb(port + LBA_MID);
    insw(port + DATA, (uint16_t *)((uint8_t *)buffer + i * 0x800), size / 2);
  }
  return false;
}