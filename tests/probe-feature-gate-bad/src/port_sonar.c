#include <stdint.h>

#define EXTI_IMR (*(volatile uint32_t *)0x40010400u)

void pm_sonar_init(void)
{
    EXTI_IMR |= (1u << 10);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}
