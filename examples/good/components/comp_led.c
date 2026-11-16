#include <stdbool.h>
#include "comp_led.h"
#include "bsp_io.h"

static bool s_led_on;
static bool s_started;

static err_t apply(bool on)
{
    err_t err = bsp_io_write(IO_LED_RUN, on);
    if (err == ERR_OK) {
        s_led_on = on;
    }
    return err;
}

err_t comp_led_set(bool on)
{
    if (!s_started) {
        return ERR_STATE;
    }
    return apply(on);
}

err_t comp_led_init(void)
{
    s_led_on = false;
    s_started = false;
    return ERR_OK;
}

err_t comp_led_start(void)
{
    s_started = true;
    return apply(false);
}

err_t comp_led_stop(void)
{
    s_started = false;
    return apply(false);
}
