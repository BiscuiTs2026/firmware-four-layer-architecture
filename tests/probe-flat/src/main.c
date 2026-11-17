#include "stm32f1xx_hal.h"
#include "logic_pid.h"

int main(void)
{
  HAL_Init();
  pid_init();
  while (1)
  {
  }
}
