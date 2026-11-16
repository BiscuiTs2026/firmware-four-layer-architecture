#include "stm32f1xx_hal.h"
#include "bsp_io.h"

static GPIO_TypeDef *const io_port[IO_MAX] = {
    [IO_LED_RUN] = GPIOA,
};

err_t bsp_io_write(io_id_t id, bool level)
{
    if (id >= IO_MAX) {
        return ERR_PARAM;
    }
    HAL_GPIO_WritePin(io_port[id], GPIO_PIN_5, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return ERR_OK;
}
