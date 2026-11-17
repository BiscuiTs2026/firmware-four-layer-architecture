#include "osal.h"
#include "stm32f1xx_hal.h"

/* HAL_GetTick 依赖 SysTick_Handler 里的 HAL_IncTick()，由 CubeMX 生成代码保证 */
uint32_t osal_tick_ms(void)
{
  return HAL_GetTick();
}

err_t osal_sleep_ms(uint32_t ms)
{
  HAL_Delay(ms);
  return ERR_OK;
}

err_t osal_init(void)
{
  return ERR_OK;
}
