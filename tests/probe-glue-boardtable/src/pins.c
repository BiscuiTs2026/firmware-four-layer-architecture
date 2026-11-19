#include <stdint.h>
#include "stm32f1xx.h"

/* 正例：板级引脚表定义在胶水层 */
static const uint16_t board_pins[] = {
    GPIO_PIN_0,
    GPIO_PIN_1,
    GPIO_PIN_2,
};

int main(void)
{
    return board_pins[0];
}
