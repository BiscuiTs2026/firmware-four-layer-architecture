#include <stdint.h>

/* 三种曾漏检的函数定义形态，都应被规则命中 */
static void __attribute__((weak)) weak_hook(void)
{
    uint32_t x = 0u;
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
    (void)x;
}

__attribute__((section(".ramfunc"))) void ram_fn(void)
{
    uint32_t x = 0u;
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
    (void)x;
}

static inline void
long_fn(void)
{
    uint32_t x = 0u;
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
    (void)x;
}

int main(void)
{
    weak_hook();
    long_fn();
    ram_fn();
    return 0;
}
