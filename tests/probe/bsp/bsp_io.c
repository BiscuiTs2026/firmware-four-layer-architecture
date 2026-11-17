#include "bsp_io.h"
#include "stm32f1xx_hal.h"

void bsp_io_write(uint8_t id, uint8_t level)
{
  (void)id;
  HAL_GPIO_WritePin(GPIOA, (uint16_t)level, GPIO_PIN_SET);
}
