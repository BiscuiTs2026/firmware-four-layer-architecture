#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include <stdint.h>
#include "common.h"

typedef enum
{
  GPIO_ID_LED = 0,
  GPIO_ID_COUNT
} gpio_id_t;

err_t bsp_gpio_init(gpio_id_t id);
err_t bsp_gpio_write(gpio_id_t id, uint8_t level);
uint8_t bsp_gpio_read(gpio_id_t id);

#endif
