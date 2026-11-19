#include <stdint.h>
#include "pm_logic.h"

/* 正例：ISR 调用组件层业务函数 + ISR 体量超限 */
void TIM2_IRQHandler(void)
{
    volatile uint32_t sink = 0u;
    sink = 1u;
    sink = 2u;
    sink = 3u;
    sink = 4u;
    sink = 5u;
    sink = 6u;
    sink = 7u;
    sink = 8u;
    sink = 9u;
    sink = 10u;
    sink = 11u;
    sink = 12u;
    sink = 13u;
    sink = 14u;
    sink = 15u;
    sink = 16u;
    sink = 17u;
    sink = 18u;
    sink = 19u;
    sink = 20u;
    sink = 21u;
    sink = 22u;
    sink = 23u;
    sink = 24u;
    sink = 25u;
    sink = 26u;
    sink = 27u;
    sink = 28u;
    sink = 29u;
    sink = 30u;
    sink = 31u;
    sink = 32u;
    sink = 33u;
    sink = 34u;
    sink = 35u;
    sink = 36u;
    sink = 37u;
    sink = 38u;
    sink = 39u;
    sink = 40u;
    sink = 41u;
    sink = 42u;
    pm_logic_step();
    (void)sink;
}

/* 反例：ISR 调 *_hw_* 钩子与端口层函数不判业务 */
void EXTI0_IRQHandler(void)
{
    dev_hw_event();
    bsp_gpio_read();
}

int main(void)
{
    return 0;
}
