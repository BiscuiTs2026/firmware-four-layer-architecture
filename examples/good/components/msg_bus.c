#include <stdint.h>
#include "msg_bus.h"
#include "osal.h"

#define BUS_SUB_MAX   8U
#define BUS_QUEUE_LEN 16U

typedef struct {
    msg_id_t      id;
    bus_handler_t handler;
} sub_t;

static sub_t   s_subs[BUS_SUB_MAX];
static uint8_t s_sub_count;

static message_t         s_queue[BUS_QUEUE_LEN];
static volatile uint16_t s_head;
static volatile uint16_t s_tail;

err_t bus_subscribe(msg_id_t id, bus_handler_t handler)
{
    if (handler == NULL) {
        return ERR_NULL;
    }
    if (s_sub_count >= BUS_SUB_MAX) {
        return ERR_NOMEM;
    }
    s_subs[s_sub_count].id = id;
    s_subs[s_sub_count].handler = handler;
    s_sub_count++;
    return ERR_OK;
}

static err_t enqueue(const message_t *msg)
{
    uint16_t next = (uint16_t)((s_head + 1U) % BUS_QUEUE_LEN);
    if (next == s_tail) {
        return ERR_BUSY;
    }
    s_queue[s_head] = *msg;
    s_head = next;
    return ERR_OK;
}

err_t bus_publish(const message_t *msg)
{
    if (msg == NULL) {
        return ERR_NULL;
    }
    return enqueue(msg);
}

err_t bus_publish_from_isr(const message_t *msg)
{
    return enqueue(msg);
}

void bus_dispatch(void)
{
    uint8_t i;

    while (s_tail != s_head) {
        message_t m = s_queue[s_tail];
        s_tail = (uint16_t)((s_tail + 1U) % BUS_QUEUE_LEN);
        for (i = 0U; i < s_sub_count; i++) {
            if ((s_subs[i].id == m.id) && (s_subs[i].handler != NULL)) {
                s_subs[i].handler(&m);
            }
        }
    }
}
