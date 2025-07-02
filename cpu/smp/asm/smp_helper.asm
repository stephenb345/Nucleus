; smp_helper.asm - SMP assembly helper code
; Created 2024/8/16 by Stephen Byrne

; Copyright (c) 2023-2024 Steve Studios Inc. All rights reserved.

global ap_trampoline
global ap_trampoline_long_mode

[extern ap_main]
[extern get_processor]
[extern active_aps]
[extern bspdone]
[extern gdt_init]
[extern idt_init]

%define GDT 0x7000     
%define AP_TRAMPOLINE_LM 0x5000   
%define STACK 0x6000   

ap_trampoline:
    bits 16

    cli
    cld

    mov edi, [0x4000]
    mov cr3, edi 

    mov eax, cr4      
    or eax, 1 << 5           
    mov cr4, eax 

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0                 
    or eax, 1 << 31 | 1 << 0   
    mov cr0, eax  
    
    lgdt [GDT]
    jmp 0x8:AP_TRAMPOLINE_LM

ap_trampoline_long_mode:
    bits 64

    mov ax, 0x10    
    mov ds, ax           
    mov es, ax      
    mov fs, ax            
    mov gs, ax        
    mov ss, ax  

    mov rax, cr3
	mov cr3, rax
    
    mov rsp, [STACK]

    mov rbx, gdt_init
    call rbx

    mov rbx, idt_init
    call rbx

    mov rbx, get_processor
    call rbx

    mov rdi, rax 
    
    jmp ap_wait
    
ap_wait:  
    pause
    
    cmp byte [bspdone], 0
    jz ap_wait

    lock inc byte [active_aps]

    mov rbx, ap_main
    jmp rbx