; task_switcher.asm - Low level task switcher
; Created 2024/05/11 by Stephen Byrne

; Copyright (c) 2023-2024 Steve Studios Inc. All rights reserved.

section .text

global switch_to_task
global start_task_entry

start_task_entry:
    mov ax, (4 * 8) | 3
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax
 
	mov rax, rsp
	push (4 * 8) | 3
	push rax
	pushf 
	push (3 * 8) | 3
    mov rdi, rdi
    push rdi
    
    iret

switch_to_task:
    cli

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov [rdi + 32], rsp

    mov rax, [rdi + 40]
    mov cr3, rax

    mov rsp, [rdi + 32]

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    sti
    
    ret