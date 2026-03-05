ASM    = nasm
CC     = gcc
LD     = ld

CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
         -mcmodel=kernel -nostdlib -I./include
LFLAGS = -m elf_x86_64 -T tools/linker.ld --oformat binary

OBJS = build/kernel_entry.o \
       build/idt_asm.o \
       build/sched_asm.o \
       build/idt.o \
       build/pic.o \
       build/timer.o \
       build/keyboard.o \
       build/vga.o \
       build/memory.o \
       build/vmm.o \
       build/heap.o \
       build/compress.o \
       build/mcompress.o \
       build/proc.o \
       build/mouse.o \
       build/gui.o \
       build/kernel.o

all: build/myos.img
	@echo "Done! Run: make run"

build/myos.img: build/boot.bin build/kernel.bin | build
	dd if=/dev/zero          of=$@ bs=512 count=2880  2>/dev/null
	dd if=build/boot.bin     of=$@ bs=512 count=1     conv=notrunc 2>/dev/null
	dd if=build/kernel.bin   of=$@ bs=512 seek=1      conv=notrunc 2>/dev/null
	@echo "Kernel size: $$(wc -c < build/kernel.bin) bytes"

build/boot.bin: boot/boot.asm | build
	$(ASM) -f bin $< -o $@

build/kernel.bin: $(OBJS) | build
	$(LD) $(LFLAGS) $(OBJS) -o $@

build/kernel_entry.o: kernel/kernel_entry.asm | build
	$(ASM) -f elf64 $< -o $@

build/idt_asm.o: kernel/idt_asm.asm | build
	$(ASM) -f elf64 $< -o $@

build/sched_asm.o: kernel/sched_asm.asm | build
	$(ASM) -f elf64 $< -o $@

build/%.o: kernel/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

run: all
	qemu-system-x86_64 -drive file=build/myos.img,format=raw \
	    -m 64M -no-reboot -no-shutdown

clean:
	rm -rf build

.PHONY: all run clean
