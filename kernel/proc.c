#include "proc.h"
#include "memory.h"
#include "heap.h"
#include "vga.h"
#include "timer.h"
#include <stddef.h>

extern void sched_switch(cpu_ctx_t* old_ctx, cpu_ctx_t* new_ctx);

static thread_t  threads[MAX_THREADS];
static uint32_t  next_tid  = 1;
static int       current   = -1;
static int       sched_on  = 0;
static volatile int need_resched = 0;  /* set by timer IRQ, acted on at yield */

static const uint32_t quantums[NUM_PRIORITIES] = {
    QUANTUM_BACKGROUND,
    QUANTUM_USER,
    QUANTUM_SYSTEM,
    QUANTUM_REALTIME,
};

static void strncpy_s(char* dst, const char* src, int max) {
    int i = 0;
    while (i < max - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static int find_free_slot(void) {
    for (int i = 0; i < MAX_THREADS; i++)
        if (threads[i].state == THREAD_DEAD) return i;
    return -1;
}

static int pick_next(void) {
    int best = -1, best_pri = -1;
    int start = (current >= 0) ? current : 0;
    for (int pass = 0; pass < 2; pass++) {
        int from = (pass == 0) ? (start + 1) % MAX_THREADS : 0;
        int to   = (pass == 0) ? MAX_THREADS : start + 1;
        for (int i = from; i < to; i++) {
            if (threads[i].state != THREAD_READY) continue;
            if ((int)threads[i].priority > best_pri) {
                best_pri = threads[i].priority;
                best = i;
            }
        }
        if (best >= 0 && best_pri == PRIORITY_REALTIME) break;
    }
    return best;
}

static void thread_trampoline(void) {
    threads[current].entry();
    thread_exit();
    for(;;) asm volatile("hlt");
}

void sched_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i].state = THREAD_DEAD;
        threads[i].tid   = 0;
    }
    current      = -1;
    sched_on     = 0;
    need_resched = 0;
}

int thread_create(const char* name, void (*entry)(void), uint8_t priority) {
    int slot = find_free_slot();
    if (slot < 0) return -1;

    void* stack = kmalloc(THREAD_STACK_SIZE);
    if (!stack) return -1;

    uint8_t* sp = (uint8_t*)stack;
    for (int i = 0; i < THREAD_STACK_SIZE; i++) sp[i] = 0;

    thread_t* t = &threads[slot];
    t->tid           = next_tid++;
    t->state         = THREAD_READY;
    t->priority      = priority < NUM_PRIORITIES ? priority : PRIORITY_USER;
    t->base_priority = t->priority;
    t->quantum_left  = quantums[t->priority];
    t->run_ticks     = 0;
    t->spawn_tick    = timer_get_ticks();
    t->stack_base    = stack;
    t->stack_top     = (uint64_t)stack + THREAD_STACK_SIZE;
    t->entry         = entry;
    strncpy_s(t->name, name, 32);

    /* Initial context: RSP points just below a fake return address,
       RIP points to thread_trampoline. */
    uint64_t* stk = (uint64_t*)t->stack_top;
    stk--;
    *stk = 0;   /* sentinel return address */

    t->ctx.rip = (uint64_t)thread_trampoline;
    t->ctx.rsp = (uint64_t)stk;
    t->ctx.rbp = 0;
    t->ctx.rbx = 0;
    t->ctx.r12 = 0; t->ctx.r13 = 0;
    t->ctx.r14 = 0; t->ctx.r15 = 0;

    sched_on = 1;
    return (int)t->tid;
}

void thread_exit(void) {
    if (current < 0) return;

    if (threads[current].stack_base) {
        kfree(threads[current].stack_base);
        threads[current].stack_base = 0;
    }
    threads[current].state = THREAD_DEAD;

    int next = pick_next();
    if (next < 0) {
        current  = -1;
        sched_on = 0;
        return;
    }

    int prev = current;
    current  = next;
    threads[current].state        = THREAD_RUNNING;
    threads[current].quantum_left = quantums[threads[current].priority];

    sched_switch(&threads[prev].ctx, &threads[current].ctx);
}

/* Called from thread context only (NOT from IRQ).
   Saves current, picks next, switches. */
void thread_yield(void) {
    if (!sched_on || current < 0) return;

    need_resched = 0;

    /* Apply Mach-style penalty if quantum expired */
    if (threads[current].quantum_left == 0) {
        if (threads[current].priority == PRIORITY_USER &&
            threads[current].base_priority == PRIORITY_USER)
            threads[current].priority = PRIORITY_BACKGROUND;
        else
            threads[current].priority = threads[current].base_priority;
    }

    int next = pick_next();

    /* Restore priority after demotion check */
    threads[current].priority = threads[current].base_priority;

    if (next < 0 || next == current) {
        threads[current].quantum_left = quantums[threads[current].priority];
        return;
    }

    threads[current].state = THREAD_READY;
    int prev = current;
    current  = next;
    threads[current].state        = THREAD_RUNNING;
    threads[current].quantum_left = quantums[threads[current].priority];

    sched_switch(&threads[prev].ctx, &threads[current].ctx);
}

/* Called from timer IRQ - ONLY updates bookkeeping, never switches context */
void sched_tick(void) {
    if (!sched_on || current < 0) return;
    threads[current].run_ticks++;
    if (threads[current].quantum_left > 0)
        threads[current].quantum_left--;
    if (threads[current].quantum_left == 0)
        need_resched = 1;
}

void thread_block(uint32_t tid) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].tid != tid) continue;
        if (threads[i].state == THREAD_RUNNING || threads[i].state == THREAD_READY) {
            threads[i].state = THREAD_BLOCKED;
            if (i == current) thread_yield();
        }
        return;
    }
}

void thread_unblock(uint32_t tid) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].tid == tid && threads[i].state == THREAD_BLOCKED) {
            threads[i].state = THREAD_READY;
            return;
        }
    }
}

int thread_kill(uint32_t tid) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].tid != tid) continue;
        if (threads[i].state == THREAD_DEAD) return -1;
        if (i == current) { thread_exit(); return 0; }
        if (threads[i].stack_base) {
            kfree(threads[i].stack_base);
            threads[i].stack_base = 0;
        }
        threads[i].state = THREAD_DEAD;
        return 0;
    }
    return -1;
}

uint32_t sched_current_tid(void) {
    return (current >= 0) ? threads[current].tid : 0;
}

thread_t* sched_get(uint32_t tid) {
    for (int i = 0; i < MAX_THREADS; i++)
        if (threads[i].tid == tid) return &threads[i];
    return 0;
}

static const char* state_name(thread_state_t s) {
    switch(s) {
        case THREAD_READY:   return "ready  ";
        case THREAD_RUNNING: return "running";
        case THREAD_BLOCKED: return "blocked";
        case THREAD_ZOMBIE:  return "zombie ";
        default:             return "dead   ";
    }
}

static const char* pri_name(uint8_t p) {
    switch(p) {
        case PRIORITY_REALTIME:   return "realtime  ";
        case PRIORITY_SYSTEM:     return "system    ";
        case PRIORITY_USER:       return "user      ";
        case PRIORITY_BACKGROUND: return "background";
        default:                  return "?         ";
    }
}

void sched_print_all(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n--- Process List ---\n");
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_print("TID  STATE    PRIORITY    TICKS  NAME\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    int found = 0;
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_t* t = &threads[i];
        if (t->state == THREAD_DEAD) continue;
        found = 1;
        vga_print_int(t->tid); vga_print("    ");
        if (t->state == THREAD_RUNNING)
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        else if (t->state == THREAD_BLOCKED)
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        else
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_print(state_name(t->state));
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_print("  ");
        vga_print(pri_name(t->priority)); vga_print("  ");
        vga_print_int(t->run_ticks);      vga_print("      ");
        vga_print(t->name);               vga_print("\n");
    }
    if (!found) vga_print("  (no threads)\n");
}
