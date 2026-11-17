#ifndef BSP_IO_H
#define BSP_IO_H

#include <stdint.h>

#define BSP_IO_MAX 4

int bsp_io_write(uint8_t id, uint8_t level);

#endif
