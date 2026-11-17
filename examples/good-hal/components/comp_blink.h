#ifndef COMP_BLINK_H
#define COMP_BLINK_H

#include <stdint.h>
#include "common.h"

err_t comp_blink_init(uint32_t half_period_ms);
void comp_blink_task(void);

#endif
