#include "pic.h"
void pic_init(void) {
    outb(PIC1_COMMAND, PIC_INIT); io_wait();
    outb(PIC2_COMMAND, PIC_INIT); io_wait();
    outb(PIC1_DATA, PIC1_OFFSET); io_wait();
    outb(PIC2_DATA, PIC2_OFFSET); io_wait();
    outb(PIC1_DATA, 4); io_wait();
    outb(PIC2_DATA, 2); io_wait();
    outb(PIC1_DATA, 1); io_wait();
    outb(PIC2_DATA, 1); io_wait();
    /* mask all except timer(0) keyboard(1) cascade(2) */
    outb(PIC1_DATA, 0xF8);
    outb(PIC2_DATA, 0xFF);
}
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}
