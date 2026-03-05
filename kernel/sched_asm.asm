; =============================================================================
; sched_asm.asm  -  Low-level context-switch primitives
;
; void sched_switch(cpu_ctx_t* old_ctx, cpu_ctx_t* new_ctx)
;
;   Saves callee-saved registers + RIP/RSP of the CURRENT thread into *old_ctx,
;   then restores them from *new_ctx and returns into the new thread.
;
; Register layout in cpu_ctx_t  (offsets from struct base, 8 bytes each):
;   0  r15
;   8  r14
;  16  r13
;  24  r12
;  32  rbx
;  40  rbp
;  48  rip   <- we fake this as a return address
;  56  rsp
; =============================================================================

[BITS 64]
global sched_switch

sched_switch:
    ; rdi = old_ctx*,  rsi = new_ctx*

    ; --- Save current thread ---
    mov [rdi + 0],  r15
    mov [rdi + 8],  r14
    mov [rdi + 16], r13
    mov [rdi + 24], r12
    mov [rdi + 32], rbx
    mov [rdi + 40], rbp

    ; Save return address (where the old thread will resume)
    mov rax, [rsp]          ; peek at return address on stack
    mov [rdi + 48], rax     ; store as saved RIP

    ; Save stack pointer AFTER the return address slot
    lea rax, [rsp + 8]      ; rsp as it will be after ret
    mov [rdi + 56], rax

    ; --- Restore new thread ---
    mov r15, [rsi + 0]
    mov r14, [rsi + 8]
    mov r13, [rsi + 16]
    mov r12, [rsi + 24]
    mov rbx, [rsi + 32]
    mov rbp, [rsi + 40]

    ; Switch to new stack
    mov rsp, [rsi + 56]

    ; Jump to new thread's RIP
    jmp qword [rsi + 48]
