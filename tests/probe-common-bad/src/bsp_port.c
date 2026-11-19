#include "bsp_port.h"

void bsp_port_led_on(void)
{
    volatile unsigned int *odr = (volatile unsigned int *)0x40010C0C;
    *odr = 1u;
}
