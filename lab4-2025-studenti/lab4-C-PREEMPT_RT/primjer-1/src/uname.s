.intel_syntax noprefix

.section .text

.global write
.global uname

write:
    mov rax, 1
    syscall
    ret

uname:
    mov rax, 63
    syscall
    ret
