#ifndef PM_API_H
#define PM_API_H

/* 含函数定义的头：端口层仍判反向依赖（issue #3 反例） */
static inline int pm_api_do(void)
{
  return 0;
}

#endif
