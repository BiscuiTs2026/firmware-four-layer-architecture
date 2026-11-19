#include "stm32f1xx_hal.h"

static int wait_flag(void)
{
    uint32_t t = 1000u;

    while (((I2C1->CR1 & 1u) != 0u) && (t-- != 0u))
    {
    }
    return (t != 0u);
}

static void delay_ms(uint32_t ms)
{
    (void)ms;
}

int main(void)
{
    HAL_Init();
    HAL_Delay(10u);
    while (1)
    {
    }
    return 0;
}
