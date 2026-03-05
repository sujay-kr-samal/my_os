[BITS 16]
[ORG 0x7E00]

    ; Set VESA 800x600x32 with linear framebuffer
    mov ax, 0x4F02
    mov bx, 0x4115      ; mode 0x115 = 800x600x32, bit14 = use linear fb
    int 0x10

    ; Get mode info -> store at 0x9000 temporarily
    mov ax, 0x4F01
    mov cx, 0x0115
    push ds
    pop es
    mov di, 0x9000
    int 0x10

    ; Copy framebuffer base address to 0x500 for kernel
    mov eax, [0x9028]       ; PhysBasePtr at offset 0x28
    mov [0x500], eax        ; fb addr
    mov dword [0x504], 800  ; width
    mov dword [0x508], 600  ; height
    mov dword [0x50C], 32   ; bpp
    mov eax, [0x9010]       ; BytesPerScanLine at offset 0x10
    and eax, 0xFFFF
    mov [0x510], eax        ; pitch

    ; Check if VESA succeeded (fb addr should be non-zero)
    cmp dword [0x500], 0
    jne .vesa_ok
    ; Fallback: mode 13h 320x200
    mov ax, 0x0013
    int 0x10
    mov dword [0x500], 0xA0000
    mov dword [0x504], 320
    mov dword [0x508], 200
    mov dword [0x50C], 8
    mov dword [0x510], 320
    jmp .video_done
.vesa_ok:
.video_done:

    ; Enter protected mode
    cli
    lgdt [.gdtptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:.pm32

align 8
.gdt:
    dq 0
    dq 0x00CF9A000000FFFF   ; 32-bit code
    dq 0x00CF92000000FFFF   ; 32-bit data
    dq 0x00AF9A000000FFFF   ; 64-bit code
    dq 0x00AF92000000FFFF   ; 64-bit data
.gdtend:
.gdtptr:
    dw .gdtend - .gdt - 1
    dd .gdt

[BITS 32]
.pm32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x9F000

    ; Page tables at 0x1000
    mov edi, 0x1000
    mov ecx, 3*1024
    xor eax, eax
    rep stosd

    mov dword [0x1000], 0x2003
    mov dword [0x2000], 0x3003

    ; Identity map first 4GB with 2MB pages
    mov edi, 0x3000
    mov eax, 0x83
    mov ecx, 512
.fill:
    mov dword [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .fill

    ; Also map high framebuffer region (0xE0000000 area for VESA)
    ; Map it in another PDPT entry
    ; PML4[0] -> PDPT at 0x2000 (already done, maps 0-1GB)
    ; Add PDPT[1] -> PD at 0x4000 (maps 1GB-2GB)  
    mov dword [0x2008], 0x4003   ; PDPT[1]
    mov edi, 0x4000
    mov eax, 0x40000083          ; start at 1GB
    mov ecx, 512
.fill2:
    mov dword [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .fill2

    ; PDPT[3] -> PD at 0x5000 for 3GB-4GB (where VESA fb often lives)
    mov dword [0x2018], 0x5003
    mov edi, 0x5000
    mov eax, 0xC0000083
    mov ecx, 512
.fill3:
    mov dword [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .fill3

    ; Enable PAE
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax
    ; Load CR3
    mov eax, 0x1000
    mov cr3, eax
    ; Enable LME
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr
    ; Enable paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    jmp 0x18:.lm64

[BITS 64]
.lm64:
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, 0x200000
    mov rax, 0x10000
    call rax
.hang:
    cli
    hlt
    jmp .hang
