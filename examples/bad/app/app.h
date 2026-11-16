#ifndef APP_H
#define APP_H

#include <stm32f1xx_hal.h>

err_t app_init(SPI_HandleTypeDef *hspi);
void app_task(void);

#endif
