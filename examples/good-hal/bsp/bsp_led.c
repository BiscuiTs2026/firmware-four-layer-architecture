#include "bsp_led.h"
#include "stm32f1xx_hal.h"

/* 引脚映射只存在于本文件：换板子只改这里 */
#define LED_STATUS_PORT GPIOC
#define LED_STATUS_PIN  GPIO_PIN_13

static err_t led_pin(led_id_t id, GPIO_TypeDef **port, uint16_t *pin)
{
  if (port == NULL || pin == NULL)
  {
    return ERR_NULL;
  }

  switch (id)
  {
    case LED_ID_STATUS:
      *port = LED_STATUS_PORT;
      *pin = LED_STATUS_PIN;
      return ERR_OK;
    default:
      return ERR_PARAM;
  }
}

err_t bsp_led_init(led_id_t id)
{
  GPIO_TypeDef *port = NULL;
  uint16_t pin = 0U;
  err_t err = led_pin(id, &port, &pin);

  if (err != ERR_OK)
  {
    return err;
  }

  /* CubeMX 已在 MX_GPIO_Init 里配好时钟与模式，这里只保证输出初始为灭 */
  HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
  return ERR_OK;
}

err_t bsp_led_write(led_id_t id, uint8_t on)
{
  GPIO_TypeDef *port = NULL;
  uint16_t pin = 0U;
  err_t err = led_pin(id, &port, &pin);

  if (err != ERR_OK)
  {
    return err;
  }

  HAL_GPIO_WritePin(port, pin, (on != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
  return ERR_OK;
}
