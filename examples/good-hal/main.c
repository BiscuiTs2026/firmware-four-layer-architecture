#include "stm32f1xx_hal.h"
#include "app.h"
#include "bsp_led.h"
#include "osal.h"

/* 入口胶水层：只做初始化与调度，不写业务。CubeMX 生成时本文件在 USER CODE 区。 */

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
    app_run();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
