#ifndef COMPONENTS_MSG_BUS_H
#define COMPONENTS_MSG_BUS_H

#include <stdint.h>
#include "common/types.h"

typedef void (*bus_handler_t)(const message_t *msg);

/* 任务上下文发布；队列满返回 ERR_BUSY，由调用方决定丢弃或重试 */
err_t bus_publish(const message_t *msg);

/* ISR 专用：不阻塞、不等待，内部落队列 */
err_t bus_publish_from_isr(const message_t *msg);

/* 仅初始化阶段调用；handler 运行在订阅方任务上下文，禁止阻塞 */
err_t bus_subscribe(msg_id_t id, bus_handler_t handler);

/* 由主循环或总线任务调用，派发队列中的消息 */
void bus_dispatch(void);

#endif
