#include <stdio.h>

#include "stm32f1xx.h"

/* 旧版漏：ISR 名单只认 *_IRQHandler / *_isr，不认 CMSIS 的 SysTick_Handler */
void SysTick_Handler(void)
{
  printf("tick\n");
}

int main(void)
{
  for (;;)
  {
  }
}
