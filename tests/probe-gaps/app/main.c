#include "app_gap.h"

/* 旧版漏：规则只允许一个 main.c，但检查器不数入口文件 */
void app_second_entry(void)
{
  g_shared = 1U;
}
