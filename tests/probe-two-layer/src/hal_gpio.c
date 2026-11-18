#include "hal_gpio.h"

/* 反向依赖：端口层在本地声明并调用上层函数 */
extern void app_logic_run(void);

int hal_gpio_init(void)
{
  return 0;
}

void hal_gpio_write(int level)
{
  (void)level;
  app_logic_run();
}
