#include "stm32f1xx_hal.h"

static GPIO_InitTypeDef s_gpio;

int main(void)
{
    s_gpio.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOA, &s_gpio);
    *(volatile uint32_t *)0x40010C00 = 0u;
    /* GPIOB 只出现在注释里，不应命中 */
    while (1)
    {
    }
    return 0;
}
