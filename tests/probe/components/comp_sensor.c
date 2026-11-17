#include "task.h"
#include "queue.h"
#include "gd32f10x.h"
#include "comp_sensor.h"

uint8_t g_ctx;
uint8_t g_buf[4];
extern uint8_t g_shared;

/*
 * 下面这些写在注释里，检查器必须忽略：
 * HAL_GPIO_WritePin(GPIOA, 1, 1);
 * uint32_t g_ignored;
 */
void comp_sensor_init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  LL_GPIO_SetPinMode(GPIOA, 1, 1);
  gpio_bit_set(GPIOA, 1);
  vTaskDelay(10);
}
