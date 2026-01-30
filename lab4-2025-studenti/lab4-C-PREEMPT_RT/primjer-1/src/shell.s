.intel_syntax noprefix

.section .text

.global write
.global read
.global fork
.global execve
.global sys_waitid
.global _exit
.global getcwd

write:
    mov rax, 1
    syscall
    ret

read:
    xor rax, rax
    syscall
    ret

fork:
    mov rax, 57
    syscall
    ret

execve:
    mov rax, 59
    syscall
    ret

sys_waitid:
    mov rax, 247
    mov r10, rcx
    syscall
    ret

_exit:
    mov rax, 60
    syscall

getcwd:
    mov rax, 79
    syscall
    ret
