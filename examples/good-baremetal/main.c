#include "stm32f1xx.h"
#include "app.h"
#include "bsp_gpio.h"
#include "osal_tick.h"

/* 入口胶水层：寄存器初始化 + 中断分发 + 调度，不写业务。 */

uint32_t SystemCoreClock = 8000000U;

void SystemInit(void)
{
}

void SysTick_Handler(void)
{
  osal_tick_isr();
}

int main(void)
{
  (void)SysTick_Config(SystemCoreClock / 1000U);

  if (bsp_gpio_init(GPIO_ID_LED) != ERR_OK)
  {
    while (1)
    {
    }
  }

  if (app_init() != ERR_OK)
  {
    while (1)
    {
    }
  }

  while (1)
  {
    app_run();
  }
}
