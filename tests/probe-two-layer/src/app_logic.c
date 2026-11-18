#include "app_logic.h"

#include "hal_gpio.h"

void app_logic_init(void)
{
  (void)hal_gpio_init();   /* 两层工程允许：没有组件层可经 */
}

void app_logic_run(void)
{
  hal_gpio_write(1);
}
