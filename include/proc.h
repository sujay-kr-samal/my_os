#ifndef PROC_H
#define PROC_H

#include <stdint.h>
#include <stddef.h>

/* Priority bands */
#define PRIORITY_REALTIME   3
#define PRIORITY_SYSTEM     2
#define PRIORITY_USER       1
#define PRIORITY_BACKGROUND 0
#define NUM_PRIORITIES      4

/* Quantums in timer ticks (100Hz = 10ms/tick) */
#define QUANTUM_REALTIME    2
#define QUANTUM_SYSTEM      3
#define QUANTUM_USER        5
#define QUANTUM_BACKGROUND  10

#define MAX_THREADS         16
#define THREAD_STACK_SIZE   (8 * 1024)

/* Thread states */
typedef enum {
    THREAD_DEAD    = 0,
    THREAD_READY   = 1,
    THREAD_RUNNING = 2,
    THREAD_BLOCKED = 3,
    THREAD_ZOMBIE  = 4,
} thread_state_t;

/* Saved CPU context */
typedef struct {
    uint64_t r15, r14, r13, r12;
    uint64_t rbx, rbp;
    uint64_t rip;
    uint64_t rsp;
} cpu_ctx_t;

/* Thread Control Block */
typedef struct thread {
    uint32_t        tid;
    char            name[32];
    thread_state_t  state;
    uint8_t         priority;
    uint8_t         base_priority;
    uint32_t        quantum_left;
    uint64_t        run_ticks;
    uint64_t        spawn_tick;
    void*           stack_base;
    uint64_t        stack_top;
    cpu_ctx_t       ctx;
    void            (*entry)(void);
} thread_t;

/* Public API */
void      sched_init(void);
int       thread_create(const char* name, void (*entry)(void), uint8_t priority);
void      sched_tick(void);
void      thread_yield(void);
void      thread_exit(void);
void      thread_block(uint32_t tid);
void      thread_unblock(uint32_t tid);
int       thread_kill(uint32_t tid);
void      sched_print_all(void);
thread_t* sched_get(uint32_t tid);
uint32_t  sched_current_tid(void);

#endif /* PROC_H */
