// elf_loader.c - ELF program loader
// Created 2023/11/09 by Stephen Byrne

#include "elfloader.h"

bool elf_check_file(Elf64_Ehdr *hdr) {
  if (!hdr)
    return false;
  if (hdr->e_ident[EI_MAG0] != ELFMAG0) {
    SYSLOG(ELFLoader, Invalid EI_MAG0)
    return false;
  }
  if (hdr->e_ident[EI_MAG1] != ELFMAG1) {
    SYSLOG(ELFLoader, Invalid EI_MAG1)
    return false;
  }
  if (hdr->e_ident[EI_MAG2] != ELFMAG2) {
    SYSLOG(ELFLoader, Invalid EI_MAG2)
    return false;
  }
  if (hdr->e_ident[EI_MAG3] != ELFMAG3) {
    SYSLOG(ELFLoader, Invalid EI_MAG3)
    return false;
  }
  return true;
}

bool elf_check_supported(Elf64_Ehdr *hdr) {
  if (!elf_check_file(hdr)) {
    SYSLOG(ELFLoader, File failed elf_check_file)
    return false;
  }
  if (hdr->e_ident[EI_CLASS] != ELFCLASS64) {
    SYSLOG(ELFLoader, File class is not ELFCLASS64(unsupported))
    return false;
  }
  if (hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
    SYSLOG(ELFLoader, File byte order is not ELFDATA2LSB(unsupported))
    return false;
  }
  if (hdr->e_machine != EM_X86_64) {
    SYSLOG(ELFLoader, File target is not EM_X86_64(unsupported))
    return false;
  }
  if (hdr->e_ident[EI_VERSION] != EV_CURRENT) {
    SYSLOG(ELFLoader, Version is unsupported)
    return false;
  }
  if (hdr->e_type != ET_REL && hdr->e_type != ET_EXEC) {
    SYSLOG(ELFLoader, Invalid file type)
    return false;
  }
  return true;
}

Elf64_Shdr *elf_sheader(Elf64_Ehdr *hdr) {
  return (Elf64_Shdr *)(hdr + hdr->e_shoff);
}

Elf64_Shdr *elf_section(Elf64_Ehdr *hdr, int idx) {
  return &elf_sheader(hdr)[idx];
}

int elf_load_stage1(Elf64_Ehdr *hdr) {
  Elf64_Shdr *shdr = elf_sheader(hdr);

  unsigned int i;
  for (i = 0; i < hdr->e_shnum; i++) {
    Elf64_Shdr *section = &shdr[i];
    if (section->sh_type == SHT_NOBITS) {
      if (!section->sh_size)
        continue;
      if (section->sh_flags & SHF_ALLOC) {
        void *mem = malloc(section->sh_size);
        memset(mem, 0, section->sh_size);
        section->sh_offset = ((uint64_t)mem - (uint64_t)hdr);
      }
    }
  }
  return 0;
}

int elf_do_reloc(Elf64_Ehdr *hdr, Elf64_Rel *rel, Elf64_Shdr *reltab);
int elf_load_stage2(Elf64_Ehdr *hdr) {
  Elf64_Shdr *shdr = elf_sheader(hdr);

  unsigned int i, idx;
  for (i = 0; i < hdr->e_shnum; i++) {
    Elf64_Shdr *section = &shdr[i];
    if (section->sh_type == SHT_REL) {
      for (idx = 0; idx < section->sh_size / section->sh_entsize; idx++) {
        Elf64_Rel *reltab = &(
            (Elf64_Rel *)((uint64_t)hdr + (uint64_t)(section->sh_offset)))[idx];
        int result = elf_do_reloc(hdr, reltab, section);
        if (result == ELF_RELOC_ERR) {
          SYSLOG(ELFLoader, Relocation error (could not relocate symbol))
          return ELF_RELOC_ERR;
        }
      }
    }
  }
  return 0;
}

void *elf_load_rel(Elf64_Ehdr *hdr) {
  int result;
  result = elf_load_stage1(hdr);
  if (result == ELF_RELOC_ERR) {
    SYSLOG(ELFLoader, Load error)
    return NULL;
  }
  result = elf_load_stage2(hdr);
  if (result == ELF_RELOC_ERR) {
    SYSLOG(ELFLoader, Relocation error)
    return NULL;
  }
  return (void *)hdr->e_entry;
}

void *elf_load_exec(Elf64_Ehdr *elf, void *buf) {
  Elf64_Phdr *phdr;
  int i;
  uintptr_t entry = 0;

  if (!memcmp(elf->e_ident, ELFMAG, SELFMAG) &&
      elf->e_ident[EI_CLASS] == ELFCLASS64 &&
      elf->e_ident[EI_DATA] == ELFDATA2LSB && elf->e_type == ET_EXEC &&
      elf->e_machine == EM_MACH && elf->e_phnum > 0) {
    for (phdr = (Elf64_Phdr *)(buf + elf->e_phoff), i = 0; i < elf->e_phnum;
         i++, phdr = (Elf64_Phdr *)((uint8_t *)phdr + elf->e_phentsize)) {
      if (phdr->p_type == PT_LOAD) {
        memcpy((void *)phdr->p_vaddr, buf + phdr->p_offset, phdr->p_filesz);
        memset((void *)(phdr->p_vaddr + phdr->p_filesz), 0,
               phdr->p_memsz - phdr->p_filesz);
      }
    }
    entry = elf->e_entry;
  }
  return (void *)entry;
}

void *elf_load_file(void *file) {
  Elf64_Ehdr *hdr = (Elf64_Ehdr *)file;
  if (!elf_check_supported(hdr)) {
    SYSLOG(ELFLoader, File not supported)
    return NULL;
  }
  switch (hdr->e_type) {
  case ET_EXEC:
    return elf_load_exec(hdr, file);
  case ET_REL:
    return elf_load_rel(hdr);
  }
  return NULL;
}

char *elf_str_table(Elf64_Ehdr *hdr) {
  if (hdr->e_shstrndx == SHN_UNDEF)
    return NULL;
  return (char *)hdr + elf_section(hdr, hdr->e_shstrndx)->sh_offset;
}

char *elf_lookup_string(Elf64_Ehdr *hdr, int offset) {
  char *strtab = elf_str_table(hdr);
  if (strtab == NULL)
    return NULL;
  return strtab + offset;
}

int elf_get_symval(Elf64_Ehdr *hdr, int table, unsigned int idx) {
  if (table == SHN_UNDEF || idx == SHN_UNDEF)
    return 0;
  Elf64_Shdr *symtab = elf_section(hdr, table);

  uint64_t symtab_entries = symtab->sh_size / symtab->sh_entsize;
  if (idx >= symtab_entries) {
    SYSLOG(ELFLoader, Symbol index out of bounds)
    return ELF_RELOC_ERR;
  }

  uint64_t symaddr = (uint64_t)hdr + (uint64_t)symtab->sh_offset;

  Elf64_Sym *symbol = &((Elf64_Sym *)symaddr)[idx];
  if (symbol->st_shndx == SHN_UNDEF) {

    if (ELF64_ST_BIND(symbol->st_info) & STB_WEAK)
      return 0;
    else {
      SYSLOG(ELFLoader, External symbol is undefined)
      return ELF_RELOC_ERR;
    }
  }
  return 0;
}

#define DO_x86_64_64(S, A) ((S) + (A))
#define DO_x86_64_PC64(S, A, P) ((S) + (A) - (P))

int elf_do_reloc(Elf64_Ehdr *hdr, Elf64_Rel *rel, Elf64_Shdr *reltab) {
  Elf64_Shdr *target = elf_section(hdr, reltab->sh_info);

  uint64_t addr = (uint64_t)hdr + (uint64_t)(target->sh_offset);
  int *ref = (int *)(addr + rel->r_offset);

  int symval = 0;
  if (ELF64_R_SYM(rel->r_info) != SHN_UNDEF) {
    symval = elf_get_symval(hdr, reltab->sh_link, ELF64_R_SYM(rel->r_info));
    if (symval == ELF_RELOC_ERR)
      return ELF_RELOC_ERR;
  }

  switch (ELF64_R_TYPE(rel->r_info)) {
  case R_X86_64_NONE:
    break;
  case R_X86_64_64:
    *ref = DO_x86_64_64(symval, *ref);
    break;
  case R_X86_64_PC64:
    *ref = DO_x86_64_PC64(symval, *ref, (uint64_t)ref);
    break;
  default:
    SYSLOG(ELFLoader, Relocation type not supported)
    return ELF_RELOC_ERR;
  }
  return symval;
}