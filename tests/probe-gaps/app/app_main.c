#include "app_gap.h"
#include "comp_led_api.h"

/*
 * 对抗夹具：旧版检查器对这些写法全部漏报。
 * 每一条都是真实工程里出现过的"看起来合规"的越层方式。
 */
void app_tick(void)
{
  RCC->APB1ENR |= 1U;                      /* 旧版漏：只认 ODR/CR 等，不认 ENR */
  SysTick->VAL = 0U;                       /* 旧版漏：SysTick 计数寄存器 */
  SCB->VTOR = 0x08000000U;                 /* 旧版漏：内核寄存器 */
  *(volatile uint32_t *)0x4001080C = 1U;   /* 旧版漏：裸地址 MMIO */
  WRITE_STATUS_LED(1U);                    /* 旧版漏：宏把 BSP 调用藏起来了 */
}
