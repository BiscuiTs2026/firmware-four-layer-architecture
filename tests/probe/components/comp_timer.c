#include <stdint.h>
#include <stdio.h>

void TIM2_IRQHandler(void)
{
  printf("irq\n");
  delay_ms(1);
}
