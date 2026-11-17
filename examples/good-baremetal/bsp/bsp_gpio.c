#include "bsp_gpio.h"
#include "stm32f1xx.h"

/* 引脚映射与寄存器操作只允许出现在 bsp/ 内 */

err_t bsp_gpio_init(gpio_id_t id)
{
  if (id != GPIO_ID_LED)
  {
    return ERR_PARAM;
  }

  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
  GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
  GPIOC->CRH |= GPIO_CRH_MODE13_1;
  GPIOC->BSRR = GPIO_BSRR_BS13;   /* 低电平点亮，初始给高电平 */

  return ERR_OK;
}

err_t bsp_gpio_write(gpio_id_t id, uint8_t level)
{
  if (id != GPIO_ID_LED)
  {
    return ERR_PARAM;
  }

  if (level != 0U)
  {
    GPIOC->BSRR = GPIO_BSRR_BS13;
  }
  else
  {
    GPIOC->BSRR = GPIO_BSRR_BR13;
  }

  return ERR_OK;
}

uint8_t bsp_gpio_read(gpio_id_t id)
{
  if (id != GPIO_ID_LED)
  {
    return 0U;
  }

  return (uint8_t)((GPIOC->IDR & GPIO_IDR_IDR13) != 0U);
}
