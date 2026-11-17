#include "stm32f1xx_hal.h"

void misc_tick(void)
{
  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
}
