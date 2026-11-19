#include <stdint.h>

static uint32_t step(uint32_t x)
{
    x += 1u;
    x += 2u;
    x += 3u;
    x += 4u;
    x += 5u;
    x += 6u;
    x += 7u;
    x += 8u;
    x += 9u;
    x += 10u;
    x += 11u;
    x += 12u;
    x += 13u;
    x += 14u;
    x += 15u;
    x += 16u;
    x += 17u;
    x += 18u;
    x += 19u;
    x += 20u;
    x += 21u;
    x += 22u;
    x += 23u;
    x += 24u;
    return x;
}

int main(void)
{
    return (int)step(0u);
}
