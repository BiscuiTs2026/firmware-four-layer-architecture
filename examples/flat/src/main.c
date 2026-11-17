/*
 * 扁平原型工程的入口胶水：只做初始化与主循环调度。
 * 分层归属写在 .arch-roles 里（本文件 = glue），不靠目录名。
 */
#include <stdint.h>

#include "board_config.h"
#include "i2c_soft.h"
#include "telemetry.h"
#include "stm32f1xx.h"

uint32_t SystemCoreClock = BOARD_MCU_HZ;

void SystemInit(void)
{
}

void SysTick_Handler(void)
{
  /* 本 ISR 只做节拍计数，业务全部在主循环推进 */
}

int main(void)
{
  char frame[TELEM_FRAME_MAX];
  uint32_t next_sample_ms = 0U;
  uint32_t now_ms = 0U;
  uint32_t since_frame = 0U;

  (void)SysTick_Config(SystemCoreClock / BOARD_TICK_HZ);
  (void)i2c_soft_init();
  telemetry_init();

  while (1)
  {
    now_ms++;

    if (now_ms < next_sample_ms)
    {
      continue;
    }

    next_sample_ms += BOARD_SAMPLE_MS;
    if ((now_ms - next_sample_ms) > BOARD_SAMPLE_MS)
    {
      next_sample_ms = now_ms + BOARD_SAMPLE_MS;
    }

    telemetry_sample();
    since_frame++;

    if (since_frame >= BOARD_FRAME_EVERY)
    {
      since_frame = 0U;
      (void)telemetry_frame(frame, (uint16_t)sizeof(frame));
    }
  }
}
