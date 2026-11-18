#include "stm32f1xx_hal.h"
#include "app.h"
#include "bsp_led.h"
#include "osal.h"

/* 入口胶水层：只做初始化与调度，不写业务。CubeMX 生成时本文件在 USER CODE 区。 */

void Error_Handler(void);

static void SystemClock_Config(void)
{
  /* 由 CubeMX 生成，示例省略 */
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  /* MX_GPIO_Init() / MX_USART1_UART_Init() 等由 CubeMX 生成，示例省略 */

  if (osal_init() != ERR_OK)
  {
    Error_Handler();
  }

  if (bsp_led_init(LED_ID_STATUS) != ERR_OK)
  {
    Error_Handler();
  }

  if (app_init() != ERR_OK)
  {
    Error_Handler();
  }

  while (1)
  {
    if (app_run() != ERR_OK)
    {
      Error_Handler();
    }
  }
}

/*
 * 故障出口：保持中断开启，LED 快闪并在 s_error_edges 里累计翻转次数。
 * 不关中断，调试器仍可连接并读变量；满足规范 7.4（禁止静默死循环）。
 */
void Error_Handler(void)
{
  static volatile uint32_t s_error_edges = 0U;

  for (;;)
  {
    if (bsp_led_write(LED_ID_STATUS, 1U) == ERR_OK)
    {
      s_error_edges++;
    }
    for (volatile uint32_t i = 0U; i < 200000U; i++)
    {
    }

    if (bsp_led_write(LED_ID_STATUS, 0U) == ERR_OK)
    {
      s_error_edges++;
    }
    for (volatile uint32_t i = 0U; i < 200000U; i++)
    {
    }
  }
}
