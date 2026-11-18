#include "bsp_gap.h"

/*
 * 旧版漏：反向依赖只查 #include。
 * BSP 在本地写 extern 声明就能反调组件，绕过 include 检查。
 */
extern void comp_secret(void);

void bsp_gap_init(void)
{
  comp_secret();
}

void bsp_status_set(uint8_t on)
{
  (void)on;
}
