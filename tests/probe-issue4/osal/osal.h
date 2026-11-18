#ifndef OSAL_H
#define OSAL_H

unsigned osal_critical_enter(void);
void osal_critical_exit(unsigned state);
void osal_tick_inc(void);
void osal_delay_ms(unsigned ms);

#endif
