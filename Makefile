override MAKEFLAGS += -rR
override KERNEL := nucleus_kernel

override CFLAGS += \
    -O3 \
    -g \
    -Wall \
    -std=gnu11 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fno-PIC \
    -m64 \
    -mno-80387 \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mno-red-zone \
    -target x86_64-elf \
    -Wno-gnu-variable-sized-type-not-at-end \
    -Wno-unused-but-set-variable
    
override LDFLAGS += \
    -O3 \
    -m elf_x86_64 \
    -nostdlib \
    -static \
    --no-dynamic-linker \
    -z text \
    -z max-page-size=0x1000 \
    -T link.ld

override NASMFLAGS += \
    -O3 \
    -Wall \
    -f elf64 -g

override CFILES := $(shell find -L * -type f -name '*.c')
override NASMFILES := $(shell find -L * -type f -name '*.asm')
override OBJ := $(addprefix bin/obj/,$(CFILES:.c=.c.o) $(NASMFILES:.asm=.asm.o))
override HEADER_DEPS := $(addprefix bin/obj/,$(CFILES:.c=.c.h))

.PHONY: all
all: bin/$(KERNEL)
 
bin/$(KERNEL): Makefile link.ld $(OBJ)
	mkdir -p "$$(dirname $@)"
	ld.lld $(OBJ) $(LDFLAGS) -o $@
 
-include $(HEADER_DEPS)
 
bin/obj/%.c.o: %.c Makefile
	mkdir -p "$$(dirname $@)"
	cc $(CFLAGS) $(CPPFLAGS) -c $< -o $@
 
bin/obj/%.asm.o: %.asm Makefile
	mkdir -p "$$(dirname $@)"
	nasm $(NASMFLAGS) $< -o $@