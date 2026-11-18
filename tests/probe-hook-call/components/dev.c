#include "dev.h"

void dev_hw_event(dev_t *d)
{
  d->state = 1;
}

int dev_start(dev_t *d)
{
  return d->state;
}
