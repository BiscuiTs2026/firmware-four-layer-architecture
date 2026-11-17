#include "app.h"
#include "stm32f1xx_hal.h"
#include "bsp_io.h"

uint32_t g_app_state;
extern uint32_t g_shared;

void app_run(void)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
  GPIOA->ODR = 1U;
  bsp_io_write(0U, 1U);
  osal_queue_send(0, 0);

  if (g_app_state != 0U)
  {
    if (g_shared != 0U)
    {
      if (g_shared > 2U)
      {
        if (g_app_state == 3U)
        {
          g_shared = 4U;
        }
      }
    }
  }
}
