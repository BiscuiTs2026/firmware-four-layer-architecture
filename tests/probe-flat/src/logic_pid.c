#include "logic_pid.h"
#include "stm32f1xx_hal.h"

int32_t g_pid_out;

void pid_init(void)
{
  g_pid_out = 0;
  GPIOA->ODR = 0U;
}
