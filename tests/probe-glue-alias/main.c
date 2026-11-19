#include "stm32f1xx_hal.h"

#define LED_PORT   GPIOA
#define LED_PIN    GPIO_PIN_5
#define UNUSED_PORT GPIOB

int main(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, 1);
    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    return 0;
}
