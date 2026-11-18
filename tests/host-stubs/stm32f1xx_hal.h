/*
 * 主机侧语法检查用的最小 HAL 头（stub）。
 * 只声明 examples/good-hal 用到的符号；不实现行为，也不替代 STM32CubeF1。
 */
#ifndef HOST_STUB_STM32F1XX_HAL_H
#define HOST_STUB_STM32F1XX_HAL_H

#include <stddef.h>
#include <stdint.h>

#include "stm32f1xx.h"

typedef enum
{
  GPIO_PIN_RESET = 0,
  GPIO_PIN_SET = 1
} GPIO_PinState;

#define GPIO_PIN_13 ((uint16_t)0x2000U)

void HAL_Init(void);
void HAL_Delay(uint32_t ms);
uint32_t HAL_GetTick(void);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);

#endif
