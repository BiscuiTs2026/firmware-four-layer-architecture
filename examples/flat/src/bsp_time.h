#ifndef BSP_TIME_H
#define BSP_TIME_H

#include <stdint.h>

/*
 * BSP 内部微秒级忙等：只给软件 I2C、单总线等位时序用。
 * BSP 不允许依赖 OSAL，所以这类时序原语由 BSP 自带；
 * 换主频时只改 bsp_time.c 里的校准系数。
 */
void bsp_delay_us(uint16_t us);

#endif
