#include "comp_bad.h"

#include "bsp_gap.h"

typedef struct
{
  uint32_t magic;
} gap_snap_t;

/* 正例：段名在 .ld 里显式定义，不应报错 */
volatile gap_snap_t g_gap __attribute__((section(".gap_a"), used));

void comp_secret(void)
{
  /* 旧版漏：厂商类型只查 .h，.c 里照样可以用 */
  GPIO_TypeDef *port = 0;
  (void)port;
}

void comp_led_api_init(void)
{
}
