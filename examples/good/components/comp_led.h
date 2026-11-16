#ifndef COMPONENTS_COMP_LED_H
#define COMPONENTS_COMP_LED_H

#include <stdbool.h>
#include "common/types.h"

err_t comp_led_init(void);
err_t comp_led_start(void);
err_t comp_led_stop(void);

/* 对外语义：开 / 关运行灯，不暴露引脚 */
err_t comp_led_set(bool on);

#endif
