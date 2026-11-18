#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdint.h>

/* OSAL 时基：SysTick 中断里调 timebase_tick_isr，其余层只读 timebase_ms */
void timebase_tick_isr(void);
uint32_t timebase_ms(void);

#endif
