// ide.c - ATA IDE Driver
// Created 2024/08/23 by Stephen Byrne

#include "ide.h"

struct ide_channel channels[2];

struct ide_device ide_devices[4];

bool ide_ready = false;

unsigned char ide_buf[2048] = {0};
unsigned static char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void ide_write_reg(unsigned char channel, unsigned char reg,
                   unsigned char data) {
  if (reg > 0x07 && reg < 0x0C)
    ide_write_reg(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
  if (reg < 0x08)
    outb(channels[channel].base + reg - 0x00, data);
  else if (reg < 0x0C)
    outb(channels[channel].base + reg - 0x06, data);
  else if (reg < 0x0E)
    outb(channels[channel].ctrl + reg - 0x0A, data);
  else if (reg < 0x16)
    outb(channels[channel].bmide + reg - 0x0E, data);
  if (reg > 0x07 && reg < 0x0C)
    ide_write_reg(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

unsigned char ide_read_reg(unsigned char channel, unsigned char reg) {
  unsigned char result = 0;
  if (reg > 0x07 && reg < 0x0C)
    ide_write_reg(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
  if (reg < 0x08)
    result = inb(channels[channel].base + reg - 0x00);
  else if (reg < 0x0C)
    result = inb(channels[channel].base + reg - 0x06);
  else if (reg < 0x0E)
    result = inb(channels[channel].ctrl + reg - 0x0A);
  else if (reg < 0x16)
    result = inb(channels[channel].bmide + reg - 0x0E);
  if (reg > 0x07 && reg < 0x0C)
    ide_write_reg(channel, ATA_REG_CONTROL, channels[channel].nIEN);
  return result;
}

void insl(unsigned reg, unsigned int *buffer, int quads) {
  int index;
  for (index = 0; index < quads; index++) {
    buffer[index] = inl(reg);
  }
}

void ide_read_buffer(unsigned char channel, unsigned char reg,
                     unsigned int buffer, unsigned int quads) {
  if (reg > 0x07 && reg < 0x0C)
    ide_write_reg(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);

  asm volatile("pushq %%rsi;"
               "movw %%ds, %%si;"
               "movw %%si, %%es;"
               "popq %%rsi;"
               :
               :
               : "rsi", "memory");

  if (reg < 0x08)
    insl(channels[channel].base + reg - 0x00, &buffer, quads);
  else if (reg < 0x0C)
    insl(channels[channel].base + reg - 0x06, &buffer, quads);
  else if (reg < 0x0E)
    insl(channels[channel].ctrl + reg - 0x0A, &buffer, quads);
  else if (reg < 0x16)
    insl(channels[channel].bmide + reg - 0x0E, &buffer, quads);

  asm volatile("popq %%rsi;"
               "movw %%si, %%es;"
               :
               :
               : "rsi", "memory");

  if (reg > 0x07 && reg < 0x0C)
    ide_write_reg(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

void ide_initialize(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2,
                    unsigned int BAR3, unsigned int BAR4, uint32_t bus, uint32_t slot,
                                          uint32_t func) {

  int j, k, count = 0;

  channels[ATA_PRIMARY].base = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
  channels[ATA_PRIMARY].ctrl = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
  channels[ATA_SECONDARY].base = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
  channels[ATA_SECONDARY].ctrl = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
  channels[ATA_PRIMARY].bmide = (BAR4 & 0xFFFFFFFC) + 0;
  channels[ATA_SECONDARY].bmide = (BAR4 & 0xFFFFFFFC) + 8;

  ide_write_reg(ATA_PRIMARY, ATA_REG_CONTROL, 2);
  ide_write_reg(ATA_SECONDARY, ATA_REG_CONTROL, 2);

  int i;

  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++) {
      unsigned char err = 0, type = IDE_ATA, status;
      ide_devices[count].Reserved = 0;

      ide_write_reg(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
      msleep(1);

      ide_write_reg(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
      msleep(1);

      if (ide_read_reg(i, ATA_REG_STATUS) == 0)
        continue;

      while (1) {
        status = ide_read_reg(i, ATA_REG_STATUS);
        if ((status & ATA_SR_ERR)) {
          err = 1;
          break;
        }
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
          break;
      }

      if (err != 0) {
        unsigned char cl = ide_read_reg(i, ATA_REG_LBA1);
        unsigned char ch = ide_read_reg(i, ATA_REG_LBA2);

        if (cl == 0x14 && ch == 0xEB)
          type = IDE_ATAPI;
        else if (cl == 0x69 && ch == 0x96)
          type = IDE_ATAPI;
        else
          continue;

        ide_write_reg(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
        msleep(1);
      }

      ide_read_buffer(i, ATA_REG_DATA, (unsigned long)ide_buf, 128);

      ide_devices[count].Reserved = 1;
      ide_devices[count].Type = type;
      ide_devices[count].Channel = i;
      ide_devices[count].Drive = j;
      ide_devices[count].Signature =
          *((unsigned short *)(ide_buf + ATA_IDENT_DEVICETYPE));
      ide_devices[count].Capabilities =
          *((unsigned short *)(ide_buf + ATA_IDENT_CAPABILITIES));
      ide_devices[count].CommandSets =
          *((unsigned int *)(ide_buf + ATA_IDENT_COMMANDSETS));

      if (ide_devices[count].CommandSets & (1 << 26))
        ide_devices[count].Size =
            *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
      else
        ide_devices[count].Size =
            *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA));

      for (k = 0; k < 40; k += 2) {
        ide_devices[count].Model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
        ide_devices[count].Model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];
      }
      ide_devices[count].Model[40] = 0;
      ide_devices[count].bus = bus;
      ide_devices[count].slot = slot;
      ide_devices[count].func = func;

      count++;
    }
}

unsigned char ide_poll(unsigned char channel, unsigned int advanced_check) {
  for (int i = 0; i < 4; i++)
    ide_read_reg(channel, ATA_REG_ALTSTATUS);

  while (ide_read_reg(channel, ATA_REG_STATUS) & ATA_SR_BSY)
    ;
  if (advanced_check) {
    unsigned char state = ide_read_reg(channel, ATA_REG_STATUS);
    if (state & ATA_SR_ERR)
      return 2;
    if (state & ATA_SR_DF)
      return 1;
    if ((state & ATA_SR_DRQ) == 0)
      return 3;
  }

  return 0;
}

unsigned char ide_ata_access(unsigned char direction, unsigned char drive,
                             unsigned int lba, unsigned char numsects,
                             unsigned short selector, void* edi) {
  unsigned char lba_mode, dma, cmd;
  unsigned char lba_io[6];
  unsigned int channel = ide_devices[drive].Channel;
  unsigned int slavebit = ide_devices[drive].Drive;
  unsigned int bus = channels[channel].base;
  unsigned int words = 256;
  unsigned short cyl, i;
  unsigned char head, sect, err;

  ide_write_reg(channel, ATA_REG_CONTROL,
                channels[channel].nIEN = (!ide_ready) + 0x02);

  if (lba >= 0x10000000) {
    lba_mode = 2;
    lba_io[0] = (lba & 0x000000FF) >> 0;
    lba_io[1] = (lba & 0x0000FF00) >> 8;
    lba_io[2] = (lba & 0x00FF0000) >> 16;
    lba_io[3] = (lba & 0xFF000000) >> 24;
    lba_io[4] = 0;
    lba_io[5] = 0;
    head = 0;
  } else if (ide_devices[drive].Capabilities & 0x200) {
    lba_mode = 1;
    lba_io[0] = (lba & 0x00000FF) >> 0;
    lba_io[1] = (lba & 0x000FF00) >> 8;
    lba_io[2] = (lba & 0x0FF0000) >> 16;
    lba_io[3] = 0;
    lba_io[4] = 0;
    lba_io[5] = 0;
    head = (lba & 0xF000000) >> 24;
  } else {
    lba_mode = 0;
    sect = (lba % 63) + 1;
    cyl = (lba + 1 - sect) / (16 * 63);
    lba_io[0] = sect;
    lba_io[1] = (cyl >> 0) & 0xFF;
    lba_io[2] = (cyl >> 8) & 0xFF;
    lba_io[3] = 0;
    lba_io[4] = 0;
    lba_io[5] = 0;
    head = (lba + 1 - sect) % (16 * 63) / (63);
  }

  dma = 0;

  while (ide_read_reg(channel, ATA_REG_STATUS) & ATA_SR_BSY)
    ;

  if (lba_mode == 0)
    ide_write_reg(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head);
  else
    ide_write_reg(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head);

  if (lba_mode == 2) {
    ide_write_reg(channel, ATA_REG_SECCOUNT1, 0);
    ide_write_reg(channel, ATA_REG_LBA3, lba_io[3]);
    ide_write_reg(channel, ATA_REG_LBA4, lba_io[4]);
    ide_write_reg(channel, ATA_REG_LBA5, lba_io[5]);
  }
  ide_write_reg(channel, ATA_REG_SECCOUNT0, numsects);
  ide_write_reg(channel, ATA_REG_LBA0, lba_io[0]);
  ide_write_reg(channel, ATA_REG_LBA1, lba_io[1]);
  ide_write_reg(channel, ATA_REG_LBA2, lba_io[2]);

  if (lba_mode == 0 && dma == 0 && direction == 0)
    cmd = ATA_CMD_READ_PIO;
  if (lba_mode == 1 && dma == 0 && direction == 0)
    cmd = ATA_CMD_READ_PIO;
  if (lba_mode == 2 && dma == 0 && direction == 0)
    cmd = ATA_CMD_READ_PIO_EXT;
  if (lba_mode == 0 && dma == 1 && direction == 0)
    cmd = ATA_CMD_READ_DMA;
  if (lba_mode == 1 && dma == 1 && direction == 0)
    cmd = ATA_CMD_READ_DMA;
  if (lba_mode == 2 && dma == 1 && direction == 0)
    cmd = ATA_CMD_READ_DMA_EXT;
  if (lba_mode == 0 && dma == 0 && direction == 1)
    cmd = ATA_CMD_WRITE_PIO;
  if (lba_mode == 1 && dma == 0 && direction == 1)
    cmd = ATA_CMD_WRITE_PIO;
  if (lba_mode == 2 && dma == 0 && direction == 1)
    cmd = ATA_CMD_WRITE_PIO_EXT;
  if (lba_mode == 0 && dma == 1 && direction == 1)
    cmd = ATA_CMD_WRITE_DMA;
  if (lba_mode == 1 && dma == 1 && direction == 1)
    cmd = ATA_CMD_WRITE_DMA;
  if (lba_mode == 2 && dma == 1 && direction == 1)
    cmd = ATA_CMD_WRITE_DMA_EXT;
  ide_write_reg(channel, ATA_REG_COMMAND, cmd);

  if (direction == 0)
    for (i = 0; i < numsects; i++) {
      if ((err = ide_poll(channel, 1)))
        return err;

      asm volatile("pushq %%rsi;"
                   "movw %%ds, %%si;"
                   "movw %%ax, %%ds;"
                   "rep insw;"
                   "movw %%si, %%ds;"
                   "popq %%rsi;"
                   :
                   : "a"(selector), "c"(words), "d"(bus), "S"(edi));

      edi += (words * 2);
    }
  else {
    for (i = 0; i < numsects; i++) {
      ide_poll(channel, 0);

      asm volatile("pushq %%rsi;"
                   "movw %%ds, %%si;"
                   "movw %%ax, %%ds;"
                   "rep outsw;"
                   "movw %%si, %%ds;"
                   "popq %%rsi;"
                   :
                   : "a"(selector), "c"(words), "d"(bus), "S"(edi));

      edi += (words * 2);
    }
    ide_write_reg(channel, ATA_REG_COMMAND,
                  (char[]){ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH,
                           ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
    ide_poll(channel, 0);
  }
  return 0;
}

bool ide_read(unsigned char drive, unsigned char numsects, unsigned int lba,
              unsigned short es, void* edi) {
  bool ret = false;
  if (drive > 3 || ide_devices[drive].Reserved == 0)
    ret = true;
  else if (((lba + numsects) > ide_devices[drive].Size) &&
           (ide_devices[drive].Type == IDE_ATA))
    ret = true;
  else {
    unsigned char err;
    if (ide_devices[drive].Type == IDE_ATA)
      err = ide_ata_access(ATA_READ, drive, lba, numsects, es, edi);
  }
  return ret;
}

bool ide_write(unsigned char drive, unsigned char numsects, unsigned int lba,
               unsigned short es, void* edi) {
  bool ret = false;
  if (drive > 3 || ide_devices[drive].Reserved == 0)
    ret = true;
  else if (((lba + numsects) > ide_devices[drive].Size) &&
           (ide_devices[drive].Type == IDE_ATA))
    ret = true;
  else {
    unsigned char err;
    if (ide_devices[drive].Type == IDE_ATA)
      err = ide_ata_access(ATA_WRITE, drive, lba, numsects, es, edi);
  }
  return ret;
}

bool ide_atapi_eject(unsigned char drive) {
  bool ret = false;

  unsigned int channel = ide_devices[drive].Channel;
  unsigned int slavebit = ide_devices[drive].Drive;
  unsigned int bus = channels[channel].base;
  unsigned char err = 0;

  ide_ready = false;

  if (drive > 3 || ide_devices[drive].Reserved == 0)
    ret = true;
  else if (ide_devices[drive].Type == IDE_ATA)
    ret = true;
  else {
    ide_write_reg(channel, ATA_REG_CONTROL,
                  channels[channel].nIEN = ide_ready = false);

    atapi_packet[0] = ATAPI_CMD_EJECT;
    atapi_packet[1] = 0x00;
    atapi_packet[2] = 0x00;
    atapi_packet[3] = 0x00;
    atapi_packet[4] = 0x02;
    atapi_packet[5] = 0x00;
    atapi_packet[6] = 0x00;
    atapi_packet[7] = 0x00;
    atapi_packet[8] = 0x00;
    atapi_packet[9] = 0x00;
    atapi_packet[10] = 0x00;
    atapi_packet[11] = 0x00;

    ide_write_reg(channel, ATA_REG_HDDEVSEL, slavebit << 4);

    for (int i = 0; i < 4; i++)
      ide_read_reg(channel, ATA_REG_ALTSTATUS);

    ide_write_reg(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);
    err = ide_poll(channel, 1);

    asm("rep outsw" ::"c"(6), "d"(bus), "S"(atapi_packet));

    while (!ide_ready)
      ;
    ide_ready = false;

    err = ide_poll(channel, 1);
    ret = (err == 1);
  }

  if (err == 3)
    err = 0;
  return ret;
}