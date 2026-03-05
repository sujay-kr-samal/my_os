# MyOS
## A 64-bit Operating System from Scratch
**Version 0.6  •  Phase 6 Complete**

---

## Overview

MyOS is a custom 64-bit operating system built entirely from scratch in x86_64 assembly and C. It boots from a raw disk image, enters 64-bit long mode, and runs a graphical desktop environment with mouse support, draggable windows, and an interactive terminal — all without any external OS, libraries, or runtime.

The project spans six complete phases, from bare-metal bootloader to a working GUI with a process scheduler and macOS-style memory compression.

---

## Quick Start

### Requirements

• NASM assembler  
• GCC (x86_64 target)  
• GNU ld (binutils)  
• QEMU (qemu-system-x86_64)  
• make  

---

### Build & Run

```bash
make clean && make && make run
```

That's it. The Makefile assembles the bootloader, compiles the kernel, links everything into a raw disk image, and launches QEMU.

---

## Project Structure

```
boot/boot.asm        Single-sector bootloader — sets VGA Mode 13h, loads kernel
kernel/kernel.c      Entry point — initialises all subsystems, launches GUI
kernel/gui.c         Full GUI: desktop, windows, taskbar, cursor, terminal (406 lines)
kernel/proc.c        Process management and preemptive scheduler
kernel/mouse.c       PS/2 mouse driver
kernel/keyboard.c    PS/2 keyboard driver with scancode translation
kernel/memory.c      Physical page allocator
kernel/vmm.c         Virtual memory manager — PML4 page tables
kernel/heap.c        Kernel heap (kmalloc/kfree)
kernel/mcompress.c   macOS-style LZ77 page compression
kernel/idt.c         Interrupt Descriptor Table setup
kernel/pic.c         8259A PIC — interrupt controller
kernel/timer.c       PIT timer — 100 Hz tick
include/font.h       8x8 bitmap font — ASCII 32–127
```

---

## Development Phases

| Phase | Description | Status |
|------|-------------|-------|
| Phase 1 | Bootloader, VGA, Physical Memory | ✅ Complete |
| Phase 2 | IDT, PIC, Keyboard, Timer | ✅ Complete |
| Phase 3 | Virtual Memory (PML4), Heap | ✅ Complete |
| Phase 4 | LZ77 Page Compression (macOS-style) | ✅ Complete |
| Phase 5 | Process Management & Scheduler | ✅ Complete |
| Phase 6 | GUI — Desktop, Windows, Mouse | ✅ Complete |

---

## GUI — Phase 6

The GUI runs in VGA Mode 13h (320×200, 256 colours) with a double-buffered backbuffer for flicker-free rendering. It uses a custom 26-colour palette tuned for a dark macOS-inspired aesthetic.

---

## Features

• Dark blue-grey desktop with sidebar widgets  
• Draggable terminal windows with macOS-style traffic-light dots (red/yellow/green)  
• Uptime clock widget with gold text  
• RAM usage bar with live percentage  
• Taskbar with centred terminal icon and live clock  
• PS/2 mouse cursor with pixel-perfect arrow shape  
• Blinking cursor in the terminal input area  
• Terminal commands: help, clear, about, mem, uptime, ps, exit  

---

## Mouse

Click the terminal icon (>) in the taskbar to open a terminal window.  

Click the red dot to close it.  

Click and drag the title bar to move the window anywhere on screen.

---

## Memory Compression — Phase 4

MyOS implements a macOS-inspired LZ77 page compression system. When memory pressure is high, 4KB pages are compressed in-place and their physical RAM is freed. On next access, a page fault triggers transparent decompression.

### Typical Results

• Zero-filled pages: ~89% compression (4096 bytes → ~450 bytes)  
• Pattern data: ~60% compression  
• Random data: incompressible (detected, not stored)

Run the `mem` command in the terminal to see live free/used RAM statistics.

---

## Terminal Commands

| Command | Description |
|--------|-------------|
| help | List all available commands |
| clear | Clear the terminal screen |
| about | Show OS version and phase status |
| mem | Display free and used RAM in KB |
| uptime | Show system uptime in seconds |
| ps | Show number of open windows |
| exit | Close the current terminal window |

---

## Technical Details

| Component | Details |
|-----------|--------|
| Architecture | x86_64 — 64-bit long mode |
| Bootloader | Single 512-byte MBR sector (NASM) |
| Video | VGA Mode 13h — 320×200, 256 colours, linear framebuffer at 0xA0000 |
| Memory | Identity-mapped first 4MB via PML4 page tables |
| Stack | 64KB kernel stack at 0x200000 |
| Interrupts | IDT with 256 entries, 8259A dual PIC, 100 Hz timer |
| Heap | Simple bump allocator with free list |
| Scheduler | Preemptive round-robin, timer-driven context switch |
| Mouse | PS/2 IRQ12, bounded to 320×200 screen |
| Keyboard | PS/2 IRQ1, US QWERTY scancode set 2 |

---

## Build Output

```
build/myos.img
```

Raw 1.44MB floppy image.

---

Built from scratch — no libc, no OS, no external libraries.
