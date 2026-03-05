[BITS 64]
[EXTERN kernel_main]
global _start

_start:
    mov rsp, 0x200000
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang
