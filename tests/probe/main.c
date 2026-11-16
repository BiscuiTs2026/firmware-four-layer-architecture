#include "stm32f1xx_hal.h"
void main_task(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    GPIOA->ODR |= 0x20;
    TIM1->CCR1 = 100;
}
