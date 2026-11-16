#ifndef BSP_IO_H
#define BSP_IO_H

#include <stdbool.h>
#include "common/types.h"

/* 上层只知道语义口，不知道是 GPIOA 还是 GPIOB */
typedef enum {
    IO_LED_RUN = 0,
    IO_LED_FAULT,
    IO_MAX
} io_id_t;

err_t bsp_io_write(io_id_t id, bool level);

#endif
