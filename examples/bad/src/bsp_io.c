#include "bsp_io.h"

#include "stm32f1xx.h"

/* BSP 层写寄存器是合法的 */
int bsp_io_write(uint8_t id, uint8_t level)
{
  if (id >= BSP_IO_MAX)
  {
    return -1;
  }

  if (level != 0U)
  {
    GPIOA->BSRR = (1UL << id);
  }
  else
  {
    GPIOA->BSRR = (1UL << (id + 16U));
  }
  return 0;
}
