#ifndef OSAL_TICK_H
#define OSAL_TICK_H

#include <stdint.h>

/* 时基：SysTick 中断里调用 osal_tick_isr，其余层只读 osal_tick_ms */
void osal_tick_isr(void);
uint32_t osal_tick_ms(void);

#endif
