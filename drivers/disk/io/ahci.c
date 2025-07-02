// sata.c - SATA AHCI Driver
// Created 2023/11/11 by Stephen Byrne

#include "ahci.h"

HBA_MEM *glob_abar;

static int check_type(HBA_PORT *port) {
  uint32_t ssts = port->ssts;

  char ipm = (ssts >> 8) & 0x0F;
  char det = ssts & 0x0F;

  if (det != HBA_PORT_DET_PRESENT)
    return AHCI_DEV_NULL;
  if (ipm != HBA_PORT_IPM_ACTIVE)
    return AHCI_DEV_NULL;

  switch (port->sig) {
  case SATA_SIG_ATAPI:
    return AHCI_DEV_SATAPI;
  case SATA_SIG_SEMB:
    return AHCI_DEV_SEMB;
  case SATA_SIG_PM:
    return AHCI_DEV_PM;
  default:
    return AHCI_DEV_SATA;
  }
}

#define HBA_PxCMD_ST 0x0001
#define HBA_PxCMD_FRE 0x0010
#define HBA_PxCMD_FR 0x4000
#define HBA_PxCMD_CR 0x8000

void start_cmd(HBA_PORT *port) {
  while (port->cmd & HBA_PxCMD_CR)
    ;

  port->cmd |= HBA_PxCMD_FRE;
  port->cmd |= HBA_PxCMD_ST;
}

void stop_cmd(HBA_PORT *port) {
  port->cmd &= ~HBA_PxCMD_ST;
  port->cmd &= ~HBA_PxCMD_FRE;

  while (1) {
    if (port->cmd & HBA_PxCMD_FR)
      continue;
    if (port->cmd & HBA_PxCMD_CR)
      continue;
    break;
  }
}

extern HBA_MEM *mem_hba;
void port_rebase(HBA_PORT *port, int portno) {
  stop_cmd(port);
  uint32_t ahci_base = (uint32_t)(uintptr_t)(mem_hba) & 0x000000000000ff00;

  port->clb = ahci_base + (portno << 10);
  port->clbu = 0;
  memset((void *)(uintptr_t)(port->clb), 0, 1024);

  port->fb = ahci_base + (32 << 10) + (portno << 8);
  port->fbu = 0;
  memset((void *)(uintptr_t)(port->fb), 0, 256);

  HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)(uintptr_t)(port->clb);
  for (int i = 0; i < 32; i++) {
    cmdheader[i].prdtl = 8;
    cmdheader[i].ctba = ahci_base + (40 << 10) + (portno << 13) + (i << 8);
    cmdheader[i].ctbau = 0;
    memset((void *)(uintptr_t)cmdheader[i].ctba, 0, 256);
  }

  start_cmd(port);
}

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

int find_cmdslot(HBA_PORT *port) {
  uint32_t slots = (port->sact | port->ci);
  int cmdslots = (glob_abar->cap & 0x0f00) >> 8;
  for (int i = 0; i < cmdslots; i++) {
    if ((slots & 1) == 0)
      return i;
    slots >>= 1;
  }
  return -1;
}

bool ahci_write(HBA_PORT *port, uint32_t startl, uint32_t starth,
                uint32_t count, void *buf) {
  port->is = (uint32_t)-1;
  int spin = 0;
  int slot = find_cmdslot(port);
  if (slot == -1)
    return false;

  HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)(uintptr_t)port->clb;
  cmdheader += slot;
  cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
  cmdheader->w = 1;
  cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1;

  HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL *)(uintptr_t)cmdheader->ctba;
  memset(cmdtbl, 0,
         sizeof(HBA_CMD_TBL) + (cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));

  int i;
  for (i = 0; i < cmdheader->prdtl - 1; i++) {
    cmdtbl->prdt_entry[i].dba = (uint32_t)(uintptr_t)buf;
    cmdtbl->prdt_entry[i].dbau = (uintptr_t)(buf) >> 32;
    cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1;
    buf += 8 * 1024;
    count -= 16;
  }

  cmdtbl->prdt_entry[i].dba = (uint32_t)(uintptr_t)buf;
  cmdtbl->prdt_entry[i].dbau = (uintptr_t)(buf) >> 32;
  cmdtbl->prdt_entry[i].dbc = (count << 9) - 1;

  FIS_REG_H2D *cmdfis = (FIS_REG_H2D *)(&cmdtbl->cfis);

  cmdfis->fis_type = FIS_TYPE_REG_H2D;
  cmdfis->c = 1;
  cmdfis->command = 0x35;

  cmdfis->lba0 = (uint8_t)startl;
  cmdfis->lba1 = (uint8_t)(startl >> 8);
  cmdfis->lba2 = (uint8_t)(startl >> 16);
  cmdfis->device = 1 << 6;

  cmdfis->lba3 = (uint8_t)(startl >> 24);
  cmdfis->lba4 = (uint8_t)starth;
  cmdfis->lba5 = (uint8_t)(starth >> 8);

  cmdfis->countl = (count & 0xff);
  cmdfis->counth = (count >> 8);

  while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
    spin++;
  }

  if (spin == 1000000) {
    SYSLOG(SteveStudiosAHCIDriver, Port is idle)
    return false;
  }

  port->ci = (1 << slot);
  while (true) {
    if ((port->ci & (1 << slot)) == 0)
      break;
    if (port->is & (1 << 30)) {
      SYSLOG(SteveStudiosAHCIDriver, Error writing to Disk)
      return false;
    }
  }

  if (port->is & (1 << 30)) {
    SYSLOG(SteveStudiosAHCIDriver, Error writing to Disk)
    return false;
  }
  return true;
}

bool ahci_read(HBA_PORT *port, uint32_t startl, uint32_t starth, uint32_t count,
               void *buf) {
  port->is = (uint32_t)-1;
  int spin = 0;
  int slot = find_cmdslot(port);
  if (slot == -1)
    return false;

  HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)(uintptr_t)port->clb;
  cmdheader += slot;
  cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
  cmdheader->w = 0;
  cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1;

  HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL *)(uintptr_t)(cmdheader->ctba);
  memset(cmdtbl, 0,
         sizeof(HBA_CMD_TBL) + (cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));

  int i;
  for (i = 0; i < cmdheader->prdtl - 1; i++) {
    cmdtbl->prdt_entry[i].dba = (uint32_t)(uintptr_t)buf;
    cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1;
    cmdtbl->prdt_entry[i].i = 1;
    buf += 4 * 1024;
    count -= 16;
  }

  cmdtbl->prdt_entry[i].dba = (uint32_t)(uintptr_t)buf;
  cmdtbl->prdt_entry[i].dbc = (count << 9) - 1;
  cmdtbl->prdt_entry[i].i = 1;

  FIS_REG_H2D *cmdfis = (FIS_REG_H2D *)(&cmdtbl->cfis);

  cmdfis->fis_type = FIS_TYPE_REG_H2D;
  cmdfis->c = 1;
  cmdfis->command = 0x25;

  cmdfis->lba0 = (uint8_t)startl;
  cmdfis->lba1 = (uint8_t)(startl >> 8);
  cmdfis->lba2 = (uint8_t)(startl >> 16);
  cmdfis->device = 1 << 6;

  cmdfis->lba3 = (uint8_t)(startl >> 24);
  cmdfis->lba4 = (uint8_t)starth;
  cmdfis->lba5 = (uint8_t)(starth >> 8);

  cmdfis->countl = count & 0xFF;
  cmdfis->counth = (count >> 8) & 0xFF;

  while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
    spin++;
  }

  if (spin == 1000000) {
    SYSLOG(SteveStudiosAHCIDriver, Port is idle)
    return false;
  }

  port->ci = 1 << slot;
  while (true) {
    if ((port->ci & (1 << slot)) == 0)
      break;
    if (port->is & (1 << 30)) {
      SYSLOG(SteveStudiosAHCIDriver, Error reading Disk)
      return false;
    }
  }

  if (port->is & (1 << 30)) {
    SYSLOG(SteveStudiosAHCIDriver, Error reading Disk)
    return false;
  }
  
  return true;
}

bool ahci_probe_port(HBA_MEM *abar) {
  uint32_t pi = abar->pi;

  int i = 0;
  while (i < 32) {
    if (pi & 1) {
      int dt = check_type(&abar->ports[i]);
      if (dt == AHCI_DEV_SATA) {
        glob_abar = abar;
        port_rebase(&abar->ports[i], i);
        return false;
      }
    }

    pi >>= 1;
    i++;
  }
  return true;
}