#include "dev.h"

static dev_t s_dev;

void port_irq(int e)
{
  dev_hw_event(&s_dev);
}

int port_open(void)
{
  return dev_start(&s_dev);
}
