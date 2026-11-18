#include "bsp_time.h"

#include "board_config.h"

void bsp_delay_us(uint16_t us)
{
  /* 8 MHz 下每个循环约 0.5 us；换主频时改这里的系数 */
  volatile uint32_t loops = (uint32_t)us * (BOARD_MCU_HZ / 4000000U);

  while (loops > 0U)
  {
    loops--;
  }
}
