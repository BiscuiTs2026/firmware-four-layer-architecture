#include <string.h>
#include "stm32f1xx_hal.h"

static int eq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

static int early_return(const char *cmd)
{
    if (eq(cmd, "A")) { return 1; }
    if (eq(cmd, "B")) { return 2; }
    if (eq(cmd, "C")) { return 3; }
    if (eq(cmd, "D")) { return 4; }
    if (eq(cmd, "E")) { return 5; }
    return 0;
}

static int drain(void)
{
    int n = 0;

    while (n < 3)
    {
        n++;
    }
    return n;
}

static void Error_Handler(void)
{
    for (;;)
    {
        HAL_Delay(500u);
    }
}

int main(void)
{
    HAL_Init();
    while (1)
    {
        if (early_return("A") != 1)
        {
            drain();
        }
    }
}
