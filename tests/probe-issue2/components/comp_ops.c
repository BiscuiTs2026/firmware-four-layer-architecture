#include "comp_ops.h"

static int gpio_ops(void)
{
  return 0;
}

int comp_read(void)
{
  return gpio_ops();
}

void comp_touch(void)
{
  gpio_set_bits(1u);
}
