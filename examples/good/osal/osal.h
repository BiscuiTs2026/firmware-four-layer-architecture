#ifndef OSAL_H
#define OSAL_H

#include <stdint.h>
#include "common/types.h"

/* OSAL 公共头不得出现任何芯片头或 RTOS 头 */
err_t osal_sleep_ms(uint32_t ms);
uint32_t osal_tick_ms(void);

#endif
