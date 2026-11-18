#include "hal_gpio.h"

#include "stm32f1xx_hal.h"

/* 板载 LED 在 PA6；引脚只在这一层出现 */
#define HAL_LED_PORT GPIOA
#define HAL_LED_PIN  GPIO_PIN_6

int hal_gpio_init(void)
{
  /* 目标板上这里打开 GPIO 时钟并配置推挽输出；主机侧只做语法检查 */
  return 0;
}

void hal_gpio_write(int level)
{
  HAL_GPIO_WritePin(HAL_LED_PORT, HAL_LED_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
