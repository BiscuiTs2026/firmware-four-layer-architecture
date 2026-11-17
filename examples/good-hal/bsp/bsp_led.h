#ifndef BSP_LED_H
#define BSP_LED_H

#include <stdint.h>
#include "common.h"

/* 语义化 ID：上层只知道"哪个灯"，不知道引脚 */
typedef enum
{
  LED_ID_STATUS = 0,
  LED_ID_COUNT
} led_id_t;

err_t bsp_led_init(led_id_t id);
err_t bsp_led_write(led_id_t id, uint8_t on);

#endif
