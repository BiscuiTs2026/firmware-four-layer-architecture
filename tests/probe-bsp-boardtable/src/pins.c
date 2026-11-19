#include <stdint.h>
#include "stm32f1xx.h"

/* 反例：同一张引脚表放在端口层，不判 */
static const uint16_t board_pins[] = {
    GPIO_PIN_0,
    GPIO_PIN_1,
    GPIO_PIN_2,
};
