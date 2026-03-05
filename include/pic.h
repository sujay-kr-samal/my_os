#ifndef PIC_H
#define PIC_H
#include <stdint.h>
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
#define PIC_EOI      0x20
#define PIC_INIT     0x11
#define PIC1_OFFSET  0x20
#define PIC2_OFFSET  0x28
#define IRQ_TIMER    0
#define IRQ_KEYBOARD 1
void pic_init(void);
void pic_send_eoi(uint8_t irq);
static inline void outb(uint16_t port, uint8_t val) { asm volatile("outb %0,%1"::"a"(val),"Nd"(port)); }
static inline uint8_t inb(uint16_t port) { uint8_t r; asm volatile("inb %1,%0":"=a"(r):"Nd"(port)); return r; }
static inline void io_wait(void) { outb(0x80,0); }
#endif
