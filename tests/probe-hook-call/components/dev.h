#ifndef DEV_H
#define DEV_H

typedef struct
{
  int state;
} dev_t;

/* ISR 侧钩子：只允许端口层在中断上下文调用（*_hw_* 命名） */
void dev_hw_event(dev_t *d);

/* 组件公开 API：端口层不得调用 */
int dev_start(dev_t *d);

#endif
