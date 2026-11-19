#include <stdint.h>
#include "stm32f1xx_hal.h"
#include "bsp_port.h"

int32_t common_scale(int32_t v)
{
    HAL_Init();
    GPIOA->ODR = 1u;
    bsp_port_led_on();
    return v * 2;
}

void common_kernel(void)
{
    NVIC_EnableIRQ(1u);
}

int32_t common_mode(void)
{
    return PM_CTX->mode;
}
