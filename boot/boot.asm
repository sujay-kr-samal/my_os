; =============================================================================
; MyOS Bootloader - 64-bit (boot.asm)
; Boot sequence: Real Mode -> Protected Mode -> Long Mode (64-bit) -> Kernel
; =============================================================================

[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov si, msg_hello
    call print_string
    ; Switch to VGA Mode 13h (320x200 256-color)
    mov ax, 0x0013
    int 0x10
    call load_kernel
    call check_long_mode
    call switch_to_pm
    jmp $

print_string:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp .loop
.done:
    popa
    ret

load_kernel:
    mov si, msg_loading
    call print_string
    mov ah, 0x02
    mov al, 100
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x80
    mov bx, 0x1000
    mov es, bx
    mov bx, 0x0000
    int 0x13
    jc disk_error
    mov si, msg_loaded
    call print_string
    ret

disk_error:
    mov si, msg_disk_err
    call print_string
    jmp $

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    mov eax, 0x80000001
    cpuid
    test edx, (1 << 29)
    jz .no_long_mode
    ret
.no_long_mode:
    mov si, msg_no64
    call print_string
    jmp $

; --- 32-bit GDT (temporary) ---
gdt32_start:
    dq 0x0
gdt32_code:
    dw 0xFFFF, 0x0000
    db 0x00, 10011010b, 11001111b, 0x00
gdt32_data:
    dw 0xFFFF, 0x0000
    db 0x00, 10010010b, 11001111b, 0x00
gdt32_end:
gdt32_descriptor:
    dw gdt32_end - gdt32_start - 1
    dd gdt32_start
CODE32_SEG equ gdt32_code - gdt32_start
DATA32_SEG equ gdt32_data - gdt32_start

switch_to_pm:
    cli
    lgdt [gdt32_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE32_SEG:init_pm32

[BITS 32]
init_pm32:
    mov ax, DATA32_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x90000
    call setup_page_tables
    call switch_to_lm

setup_page_tables:
    ; Clear tables at 0x1000-0x3FFF
    mov edi, 0x1000
    mov ecx, 3 * 4096 / 4
    xor eax, eax
    rep stosd
    ; PML4[0] -> PDPT at 0x2000
    mov dword [0x1000], 0x2003
    ; PDPT[0] -> PD at 0x3000
    mov dword [0x2000], 0x3003
    ; PD: 512 x 2MB huge pages (identity map 1GB)
    mov edi, 0x3000
    mov eax, 0x83
    mov ecx, 512
.fill:
    mov dword [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .fill
    ret

switch_to_lm:
    mov eax, 0x1000
    mov cr3, eax
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr
    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax
    lgdt [gdt64_descriptor]
    jmp CODE64_SEG:init_lm64

; --- 64-bit GDT ---
gdt64_start:
    dq 0x0
gdt64_code:
    dq 0x00AF9A000000FFFF
gdt64_data:
    dq 0x00AF92000000FFFF
gdt64_end:
gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start
CODE64_SEG equ gdt64_code - gdt64_start
DATA64_SEG equ gdt64_data - gdt64_start

[BITS 64]
init_lm64:
    mov ax, DATA64_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov rsp, 0x200000
    mov rax, 0x10000
    call rax
    jmp $

msg_hello    db "MyOS 64-bit Bootloader", 0x0D, 0x0A, 0
msg_loading  db "Loading kernel...", 0x0D, 0x0A, 0
msg_loaded   db "Kernel loaded!", 0x0D, 0x0A, 0
msg_disk_err db "DISK ERROR!", 0x0D, 0x0A, 0
msg_no64     db "ERROR: CPU does not support 64-bit!", 0x0D, 0x0A, 0

times 510-($-$$) db 0
dw 0xAA55
