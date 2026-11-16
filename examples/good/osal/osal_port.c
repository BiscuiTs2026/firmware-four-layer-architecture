#include "osal.h"

/* 移植层：裸机实现。换 RTOS 时只改这个文件，上层零改动 */
#include "stm32f1xx_hal.h"

err_t osal_sleep_ms(uint32_t ms)
{
    HAL_Delay(ms);
    return ERR_OK;
}

uint32_t osal_tick_ms(void)
{
    return HAL_GetTick();
}
