/* 厂商 SDK 片段：被 ignore 掉，不应产生任何发现 */
#include "stm32f1xx_hal.h"

void vendor_init(void)
{
  HAL_Init();
  GPIOA->ODR = 1U;
}
