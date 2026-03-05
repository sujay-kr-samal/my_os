#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
void     timer_init(void);
void     timer_handler(void);
uint64_t timer_get_ticks(void);
void     timer_sleep(uint32_t ms);
#endif
