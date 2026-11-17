#ifndef I2C_SOFT_H
#define I2C_SOFT_H

#include "types.h"

err_t i2c_soft_init(void);
err_t i2c_soft_probe(uint8_t addr7);
err_t i2c_soft_write(uint8_t addr7, const uint8_t *buf, uint16_t len);

#endif
