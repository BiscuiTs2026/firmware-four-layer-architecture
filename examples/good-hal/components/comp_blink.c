#include "comp_blink.h"
#include "bsp_led.h"
#include "osal.h"

static uint32_t s_half_period_ms = 500U;
static uint32_t s_last_toggle_ms = 0U;
static uint8_t s_level = 0U;

err_t comp_blink_init(uint32_t half_period_ms)
{
  if (half_period_ms == 0U)
  {
    return ERR_PARAM;
  }

  s_half_period_ms = half_period_ms;
  s_last_toggle_ms = osal_tick_ms();
  s_level = 0U;

  return bsp_led_write(LED_ID_STATUS, s_level);
}

void comp_blink_task(void)
{
  uint32_t now = osal_tick_ms();

  if ((now - s_last_toggle_ms) < s_half_period_ms)
  {
    return;
  }

  /* 用累加而不是 now，避免长阻塞后相位漂移 */
  s_last_toggle_ms += s_half_period_ms;
  s_level = (uint8_t)(s_level == 0U);
  (void)bsp_led_write(LED_ID_STATUS, s_level);
}
