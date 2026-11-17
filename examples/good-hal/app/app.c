#include "app.h"
#include "comp_blink.h"

err_t app_init(void)
{
  return comp_blink_init(500U);
}

void app_run(void)
{
  comp_blink_task();
}
