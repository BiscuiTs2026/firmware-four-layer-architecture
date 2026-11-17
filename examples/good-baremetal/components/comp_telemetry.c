#include "comp_telemetry.h"
#include "osal_tick.h"

/* 快照块：带段属性，写只允许在本文件内发生 */
volatile telemetry_snap_t g_telemetry __attribute__((section(".telem_ram"), used));

static uint32_t s_next_sample_ms;

err_t comp_telemetry_init(void)
{
  g_telemetry.magic = TELEM_MAGIC;
  g_telemetry.seq = 0U;
  g_telemetry.samples = 0U;
  g_telemetry.max_loop_us = 0U;
  s_next_sample_ms = osal_tick_ms();

  return ERR_OK;
}

void comp_telemetry_sample(void)
{
  uint32_t now = osal_tick_ms();

  if ((int32_t)(now - s_next_sample_ms) < 0)
  {
    return;
  }

  /* 累加节拍，只有落后超过一拍才重置基准（说明发生过长阻塞） */
  if ((int32_t)(now - s_next_sample_ms) > (int32_t)TELEM_SAMPLE_PERIOD_MS)
  {
    s_next_sample_ms = now;
  }
  else
  {
    s_next_sample_ms += TELEM_SAMPLE_PERIOD_MS;
  }

  g_telemetry.samples = g_telemetry.samples + 1U;
  g_telemetry.seq = g_telemetry.seq + 1U;
}
