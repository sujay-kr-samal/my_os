#include "kernel.h"
#include "vga.h"
#include "memory.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "timer.h"
#include "vmm.h"
#include "heap.h"
#include "mcompress.h"
#include "compress.h"
#include "proc.h"
#include "gui.h"

void kernel_main(void) {
    idt_init();
    memory_init();
    pic_init();
    timer_init();
    keyboard_init();
    vmm_init();
    heap_init();
    mcompress_init();
    sched_init();
    asm volatile("sti");
    gui_init();
    gui_run();
}
