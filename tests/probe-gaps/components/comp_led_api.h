#ifndef COMP_LED_API_H
#define COMP_LED_API_H

#include "bsp_gap.h"

/* 旧版漏：宏把下层调用包了一层，上层用宏就绕过了组件 API */
#define WRITE_STATUS_LED(v) bsp_status_set((v))

void comp_led_api_init(void);

#endif
