#ifndef OSAL_H
#define OSAL_H

#include <stdint.h>
#include "common.h"

/* 时间原语：上层不出现 HAL_GetTick / vTaskDelay */
uint32_t osal_tick_ms(void);
err_t osal_sleep_ms(uint32_t ms);
err_t osal_init(void);

#endif
