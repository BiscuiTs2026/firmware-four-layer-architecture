#include "osal_tick.h"

static volatile uint32_t s_ms;

void osal_tick_isr(void)
{
  s_ms = s_ms + 1U;
}

uint32_t osal_tick_ms(void)
{
  return s_ms;
}
