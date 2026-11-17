#ifndef COMP_TELEMETRY_H
#define COMP_TELEMETRY_H

#include <stdint.h>
#include "common.h"

/*
 * 固定地址快照：段名 .telem_ram 在 ld/app_sections.ld 里显式落在 0x20003900。
 * 用 SWD hotplug 按地址回读：STM32_Programmer_CLI -c port=SWD mode=hotplug -r32 0x20003900 8
 */
#define TELEM_SNAP_ADDR 0x20003900U
#define TELEM_MAGIC     0x54454C54U
#define TELEM_SAMPLE_PERIOD_MS 10U

typedef struct
{
  uint32_t magic;       /* 0x00 */
  uint32_t seq;         /* 0x04 */
  uint32_t samples;     /* 0x08 */
  uint32_t max_loop_us; /* 0x0C */
} telemetry_snap_t;

err_t comp_telemetry_init(void);
void comp_telemetry_sample(void);

#endif
