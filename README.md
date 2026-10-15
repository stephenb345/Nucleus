# Nucleus

Nucleus is a Super-modern, Blazing Fast, Monolithic, x86_64 Operating System Kernel. Important drivers (PCI, Main Filesystems, Peripherals, etc.) are hardcoded into the Kernel, while smaller, less significant drivers are run from userspace as high priority threads.

# Building Nucleus

## Prequisites

- `nasm`
- `make`
- `clang`

You will also need A UNIX utility set, such as GNU coreutils (This is only required if you are using a non UNIX-like Host, i.e; If you are running OS X - macOS 27 or GNU/Linux, you do not need to do this step).

# Booting Nucleus

Nucleus is built as a standard ELF64 binary. You will need to find a bootloader that can load these files natively, or you can make your own, too.
