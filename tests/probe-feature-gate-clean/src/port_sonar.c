#include <stdint.h>

#define EXTI_IMR (*(volatile uint32_t *)0x40010400u)

void pm_on_init(void)
{
    EXTI_IMR |= (1u << 0);
    NVIC_EnableIRQ(TIM6_IRQn);
}

void pm_off_init(void)
{
    EXTI_IMR |= (1u << 10);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}
