#include "app_logic.h"
#include "hal_gpio.h"

int main(void)
{
  app_logic_init();
  app_logic_run();
  return 0;
}
