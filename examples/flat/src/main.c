/*
 * 扁平原型工程的入口胶水：只做初始化与主循环调度。
 * 分层归属写在 .arch-roles 里（本文件 = glue），不靠目录名。
 */
#include <stdint.h>

#include "board_config.h"
#include "i2c_soft.h"
#include "telemetry.h"
#include "timebase.h"
#include "stm32f1xx.h"

uint32_t SystemCoreClock = BOARD_MCU_HZ;

void Error_Handler(void);

void SystemInit(void)
{
}

void SysTick_Handler(void)
{
  /* 本 ISR 只做节拍计数，业务全部在主循环推进 */
  timebase_tick_isr();
}

int main(void)
{
  char frame[TELEM_FRAME_MAX];
  uint32_t next_sample_ms;
  uint32_t now_ms;
  uint32_t since_frame = 0U;

  if (SysTick_Config(SystemCoreClock / BOARD_TICK_HZ) != 0U)
  {
    Error_Handler();
  }

  if (i2c_soft_init() != ERR_OK)
  {
    Error_Handler();
  }

  telemetry_init();
  now_ms = timebase_ms();
  next_sample_ms = now_ms;

  while (1)
  {
    now_ms = timebase_ms();

    if ((int32_t)(now_ms - next_sample_ms) < 0)
    {
      continue;
    }

    next_sample_ms += BOARD_SAMPLE_MS;
    if ((int32_t)(now_ms - next_sample_ms) > (int32_t)BOARD_SAMPLE_MS)
    {
      next_sample_ms = now_ms + BOARD_SAMPLE_MS;
    }

    telemetry_sample();
    since_frame++;

    if (since_frame >= BOARD_FRAME_EVERY)
    {
      since_frame = 0U;
      if (telemetry_frame(frame, (uint16_t)sizeof(frame)) == 0U)
      {
        Error_Handler();
      }
    }
  }
}

/*
 * 故障出口：PC13 快闪 + s_error_edges 计数，保持中断开启，供 SWD 观察。
 * 入口胶水层允许直接碰寄存器；禁止静默死循环（规范 7.4）。
 */
void Error_Handler(void)
{
  static volatile uint32_t s_error_edges = 0U;

  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
  /* PC13：推挽输出 2 MHz */
  GPIOC->CRH = (GPIOC->CRH & ~(0xFUL << 20U)) | (0x2UL << 20U);

  for (;;)
  {
    GPIOC->BSRR = (1UL << 13U);
    s_error_edges++;
    for (volatile uint32_t i = 0U; i < 200000U; i++)
    {
    }

    GPIOC->BSRR = (1UL << 29U);
    s_error_edges++;
    for (volatile uint32_t i = 0U; i < 200000U; i++)
    {
    }
  }
}
