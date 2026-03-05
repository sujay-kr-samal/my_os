[BITS 16]
[ORG 0x7C00]

    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Load stage2 (1 sector) to 0x7E00
    mov ah, 0x02
    mov al, 1
    xor ch, ch
    mov cl, 2
    xor dh, dh
    mov dl, 0x80
    xor bx, bx
    mov es, bx
    mov bx, 0x7E00
    int 0x13
    jc .fail

    ; Load kernel (sectors 3..102 = 100 sectors) to 0x10000
    mov ah, 0x02
    mov al, 100
    xor ch, ch
    mov cl, 3           ; start at sector 3
    xor dh, dh
    mov dl, 0x80
    mov bx, 0x1000
    mov es, bx
    xor bx, bx          ; ES:BX = 0x1000:0x0000 = 0x10000
    int 0x13
    jc .fail

    jmp 0x0000:0x7E00   ; jump to stage2

.fail:
    mov si, .msg
.print:
    lodsb
    or al, al
    jz .hang
    mov ah, 0x0E
    int 0x10
    jmp .print
.hang: jmp $
.msg db "Disk err",0

times 510-($-$$) db 0
dw 0xAA55
