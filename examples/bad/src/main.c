#include "stm32f1xx.h"

/* 违规 1：APP 层直接操作寄存器，且没有入口胶水标记 */
uint32_t SystemCoreClock = 8000000U;

void SysTick_Handler(void)
{
  GPIOC->BSRR = (1UL << 13U);
}
