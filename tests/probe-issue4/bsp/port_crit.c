#include "osal.h"

static volatile int s_flag;

void port_write(int v)
{
  unsigned st = osal_critical_enter();
  s_flag = v;
  osal_critical_exit(st);
}

void port_wait(void)
{
  osal_delay_ms(10);
  osal_tick_inc();
}
