#include "idt.h"
#include "vga.h"
#include <stdint.h>

extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void irq0(void);  extern void irq1(void);  extern void irq4(void);

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;

typedef struct __attribute__((packed)) {
    uint64_t r15,r14,r13,r12,r11,r10,r9,r8;
    uint64_t rbp,rdi,rsi,rdx,rcx,rbx,rax;
    uint64_t int_no, err_code, rip, cs, rflags, rsp, ss;
} regs_t;

static const char* exc_names[] = {
    "Division by Zero","Debug","NMI","Breakpoint","Overflow",
    "Bound Range","Invalid Opcode","Device Not Available",
    "Double Fault","","Invalid TSS","Segment Not Present",
    "Stack Fault","General Protection","Page Fault",""
};

void exception_handler(regs_t* r) {
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_print("\n*** KERNEL PANIC: ");
    if (r->int_no < 16) vga_print(exc_names[r->int_no]);
    else vga_print("Unknown");
    vga_print(" (INT ");
    vga_print_int(r->int_no);
    vga_print(") ***\n");
    asm volatile("cli; hlt");
}

void idt_set_gate(uint8_t n, uint64_t handler, uint16_t sel, uint8_t attr) {
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = sel;
    idt[n].ist         = 0;
    idt[n].type_attr   = attr;
    idt[n].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[n].zero        = 0;
}

void idt_init(void) {
    for (int i = 0; i < IDT_ENTRIES; i++) {
        uint64_t* e = (uint64_t*)&idt[i];
        e[0] = 0; e[1] = 0;
    }
    idt_set_gate(0,  (uint64_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint64_t)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint64_t)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint64_t)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint64_t)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint64_t)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint64_t)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint64_t)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint64_t)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint64_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint64_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint64_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint64_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint64_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint64_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint64_t)isr15, 0x08, 0x8E);
    idt_set_gate(32, (uint64_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint64_t)irq1,  0x08, 0x8E);
    idt_set_gate(44, (uint64_t)irq4,  0x08, 0x8E);  /* IRQ12 mouse */
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint64_t)idt;
    asm volatile("lidt %0" : : "m"(idt_ptr));
}
