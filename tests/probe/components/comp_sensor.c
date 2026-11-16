#include "task.h"
#include "queue.h"
#include "gd32f10x.h"
/* HAL_GPIO_WritePin(this is comment) */
spi_ctx_t g_ctx;
uint8_t g_buf[64];
extern uint8_t g_shared;
void comp_poll(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    LL_GPIO_SetPinMode(GPIOA, 5, 1);
    gpio_bit_set(GPIOA, 5);
    vTaskDelay(10);
}
