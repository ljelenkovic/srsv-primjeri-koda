.intel_syntax noprefix

.section .text

.global write

write:
    mov rax, 1
    syscall
    ret
