#include "stm32f1xx_hal.h"
#include "app.h"

int main(void)
{
  HAL_Init();
  app_run();

  while (1)
  {
  }
}
