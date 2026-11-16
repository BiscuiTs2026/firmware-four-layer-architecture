#include "app.h"
#include "bsp_io.h"

uint8_t g_temp_value;

void app_task(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    GPIOA->ODR |= 0x20;

    if (cond_a) {
        if (cond_b) {
            if (cond_c) {
                if (cond_d) {
                    bsp_io_write(IO_LED_RUN, true);
                }
            }
        }
    }
}
