#include "app.h"
#include "comp_blink.h"

err_t app_init(void)
{
  return comp_blink_init(500U);
}

err_t app_run(void)
{
  return comp_blink_task();
}
