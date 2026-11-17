#include "stm32f1xx.h"

/* 违规 6：组件层出现平台头 */

typedef struct
{
  int state;
  int value;
} ctx_t;

/* 违规 7：跨模块全局变量 */
ctx_t g_ctx;
int s_value;

/* 违规 8：extern 跨模块变量 */
extern uint8_t g_shared;

/* 违规 9：ISR 内 while + printf；违规 10：组件层越层调 HAL；
   违规 11：固定地址段 .ghost_ram 未在链接脚本定义 */
void EXTI0_IRQHandler(void)
{
  volatile ctx_t ghost __attribute__((section(".ghost_ram"), used));

  while (EXTI->PR == 0U)
  {
    printf("irq\n");
    __HAL_RCC_GPIOA_CLK_ENABLE();
    (void)ghost;
  }
}
