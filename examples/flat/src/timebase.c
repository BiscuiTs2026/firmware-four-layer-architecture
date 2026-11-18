#include "timebase.h"

/* 只做毫秒累加，不包含任何平台头；换芯片时本文件可原样保留 */
static volatile uint32_t s_ms = 0U;

void timebase_tick_isr(void)
{
  s_ms++;
}

uint32_t timebase_ms(void)
{
  return s_ms;
}
