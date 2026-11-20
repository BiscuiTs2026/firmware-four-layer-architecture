#include <stdint.h>

#define EXTI_IMR (*(volatile uint32_t *)0x40010400u)
#define DIST_TIME_BASE_READY 0

void pm_sonar_init(void)
{
#if DIST_TIME_BASE_READY
    EXTI_IMR |= (1u << 10);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
#endif
}
