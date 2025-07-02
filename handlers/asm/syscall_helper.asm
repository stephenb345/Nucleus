; syscall_helper.asm - System call helper
; Created 2024/04/07 by Stephen Byrne

; Copyright (c) 2023-2024 Steve Studios Inc. All rights reserved.

global send_syscall_return_val

send_syscall_return_val:
    mov rax, rdi
    retq