#include "stm32f1xx.h"

#include "bsp_io.h"

#define IDLE 0
#define BUSY 1

int s_mode;

/* 违规 2：APP 直调 BSP；违规 3：APP 越权调 OSAL；违规 4：阻塞延时 */
void app_task(void)
{
  bsp_io_write(0, 1);
  osal_sleep_ms(100U);
  HAL_Delay(10U);
}

/* 违规 5：决策型 if 嵌套 4 层 */
int app_decide(int a, int b, int c, int d)
{
  if (a)
  {
    if (b)
    {
      if (c)
      {
        if (d)
        {
          return 1;
        }
        else
        {
          return 2;
        }
      }
      else
      {
        return 3;
      }
    }
    else
    {
      return 4;
    }
  }
  else
  {
    return IDLE;
  }
}
