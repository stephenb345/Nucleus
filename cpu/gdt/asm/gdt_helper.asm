; gdt_helper.asm - GDT (Global Descriptor Table) helper
; Created 2023/7/21 by Stephen Byrne

; Copyright (c) 2023-2024 Steve Studios Inc. All rights reserved.

global reload_segment_registers

; Reloads the segment registers, and calls ".reload_data_segments"
reload_segment_registers:
    BITS 64
    push 0x28
    lea rax, [rel .reload_data_segments] 
    push rax 
    retfq 

; Reload the CS/Data segment registers
.reload_data_segments:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret