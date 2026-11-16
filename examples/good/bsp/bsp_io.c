#include "bsp_io.h"

/* 唯一允许出现 HAL / 引脚映射的地方 */
#include "stm32f1xx_hal.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} io_map_t;

static const io_map_t k_io_map[IO_MAX] = {
    [IO_LED_RUN]   = { GPIOA, GPIO_PIN_5 },
    [IO_LED_FAULT] = { GPIOB, GPIO_PIN_0 },
};

err_t bsp_io_write(io_id_t id, bool level)
{
    if ((unsigned)id >= (unsigned)IO_MAX) {
        return ERR_PARAM;
    }
    HAL_GPIO_WritePin(
        k_io_map[id].port,
        k_io_map[id].pin,
        level ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return ERR_OK;
}
