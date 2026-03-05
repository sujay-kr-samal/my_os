#include "timer.h"
#include "pic.h"
#include "proc.h"

static volatile uint64_t ticks = 0;

void timer_init(void) {
    uint32_t divisor = 1193182 / 100;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)(divisor >> 8));
}

void timer_handler(void) {
    ticks++;
    pic_send_eoi(IRQ_TIMER);
    /* Count the tick and set resched flag - actual switch
       happens at next thread_yield() call, NOT here.
       Calling sched_switch() from inside an IRQ handler
       corrupts the IRQ stack frame. */
    sched_tick();
}

uint64_t timer_get_ticks(void) { return ticks; }

void timer_sleep(uint32_t ms) {
    uint64_t end = ticks + (ms / 10);
    while (ticks < end) asm volatile("hlt");
}
