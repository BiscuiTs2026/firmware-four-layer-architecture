#include "stm32f1xx.h"
#include "app.h"
#include "bsp_gpio.h"
#include "osal_tick.h"

/* 入口胶水层：寄存器初始化 + 中断分发 + 调度，不写业务。 */

uint32_t SystemCoreClock = 8000000U;

void Error_Handler(void);

void SystemInit(void)
{
}

void SysTick_Handler(void)
{
  osal_tick_isr();
}

int main(void)
{
  if (SysTick_Config(SystemCoreClock / 1000U) != 0U)
  {
    Error_Handler();
  }

  if (bsp_gpio_init(GPIO_ID_LED) != ERR_OK)
  {
    Error_Handler();
  }

  if (app_init() != ERR_OK)
  {
    Error_Handler();
  }

  while (1)
  {
    app_run();
  }
}

/*
 * 故障出口：LED 快闪 + s_error_edges 计数，保持中断开启，供 SWD 观察。
 * 禁止 __disable_irq() 后静默死循环（规范 7.4）。
 */
void Error_Handler(void)
{
  static volatile uint32_t s_error_edges = 0U;

  for (;;)
  {
    if (bsp_gpio_write(GPIO_ID_LED, 1U) == ERR_OK)
    {
      s_error_edges++;
    }
    for (volatile uint32_t i = 0U; i < 200000U; i++)
    {
    }

    if (bsp_gpio_write(GPIO_ID_LED, 0U) == ERR_OK)
    {
      s_error_edges++;
    }
    for (volatile uint32_t i = 0U; i < 200000U; i++)
    {
    }
  }
}
