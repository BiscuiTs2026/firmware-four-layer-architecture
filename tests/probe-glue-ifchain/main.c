#include <stdint.h>

/* 正例：8 分支 if/else-if 链 */
static int hint(uint32_t code)
{
    if (code == 1u) { return 10; }
    else if (code == 2u) { return 20; }
    else if (code == 3u) { return 30; }
    else if (code == 4u) { return 40; }
    else if (code == 5u) { return 50; }
    else if (code == 6u) { return 60; }
    else if (code == 7u) { return 70; }
    else if (code == 8u) { return 80; }
    return 0;
}

/* 反例：早退守卫链不是命令链，不计 */
static void guard(int x)
{
    if (x == 1) return;
    if (x == 2) return;
    if (x == 3) return;
    if (x == 4) return;
    if (x == 5) return;
    if (x == 6) return;
    if (x == 7) return;
    if (x == 8) return;
}

int main(void)
{
    guard(0);
    return hint(1u);
}
