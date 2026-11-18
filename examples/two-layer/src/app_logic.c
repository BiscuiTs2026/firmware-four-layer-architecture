#include "app_logic.h"

#include "hal_gpio.h"

void app_logic_init(void)
{
  /* 两层工程：业务层直接调硬件层，没有组件层可经 */
  (void)hal_gpio_init();
}

void app_logic_run(void)
{
  hal_gpio_write(1);
}
