#include "app.h"
#include "msg_bus.h"
#include "comp_led.h"
#include "osal.h"

/* APP 层只做三件事：拿消息、判状态、调组件。不碰 BSP、不碰 HAL、不碰寄存器 */
static void on_sys_run(const message_t *msg)
{
    if ((msg == NULL) || (msg->id != MSG_SYS_RUN)) {
        return;
    }
    (void)comp_led_set(true);
}

static void on_led_req(const message_t *msg)
{
    if ((msg == NULL) || (msg->id != MSG_LED_SET) || (msg->len < 1U)) {
        return;
    }
    (void)comp_led_set(msg->payload[0] != 0U);
}

err_t app_init(void)
{
    err_t err = comp_led_init();
    if (err != ERR_OK) {
        return err;
    }
    err = bus_subscribe(MSG_SYS_RUN, on_sys_run);
    if (err != ERR_OK) {
        return err;
    }
    return bus_subscribe(MSG_LED_SET, on_led_req);
}

err_t app_start(void)
{
    err_t err = comp_led_start();
    if (err != ERR_OK) {
        return err;
    }
    /* 需要延时时走 OSAL 时间原语，不直调 BSP，也不调队列/互斥 */
    return osal_sleep_ms(10U);
}
