#ifndef PM_UART_H
#define PM_UART_H

/* 仅类型与 ISR 钩子原型：端口层允许包含（issue #3） */
typedef struct
{
  int owner;
  int opened;
} pm_uart_t;

void pm_uart_hw_rx_byte(int byte);

#endif
