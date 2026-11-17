#include "app.h"
#include "comp_telemetry.h"

err_t app_init(void)
{
  return comp_telemetry_init();
}

void app_run(void)
{
  comp_telemetry_sample();
}
