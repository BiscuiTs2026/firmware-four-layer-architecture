#include "app.h"
#include "bsp_io.h"
#include "msg_bus.h"

static void on_sys_msg(const message_t *msg)
{
    if (msg == NULL) {
        return;
    }
    if (msg->id == MSG_SYS_RUN) {
        (void)bsp_io_write(IO_LED_RUN, true);
    }
}

err_t app_init(void)
{
    return bus_subscribe(MSG_SYS_RUN, on_sys_msg);
}
