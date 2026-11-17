#include "hw_gpio.h"
#include "stm32f1xx_hal.h"

void hw_led_set(uint8_t on)
{
  if (on != 0U)
  {
    GPIOA->BSRR = (1UL << 5);
  }
  else
  {
    GPIOA->BRR = (1UL << 5);
  }
}
