#include <stdint.h>
#include <string.h>
#include "stm32f1xx.h"

/* 注释里的 GPIO_PIN_9 不算硬编码引脚（mask 后是空格） */

static void i2c_bus_fix(void)
{
    I2C1->CR1 |= 0x1u;
    I2C1->CR1 &= ~0x1u;
    I2C1->CR2 = 0x10u;
}

static void hw_init(void)
{
    /* 初始化函数天然多次写寄存器，G1 排除 */
    RCC->APB1ENR |= 0x1u;
    GPIOA->CRL = 0x4u;
    GPIOA->CRH = 0x8u;
}

void TIM2_IRQHandler(void)
{
    /* ISR 里清寄存器同样排除 G1 */
    TIM2->SR = 0u;
    TIM2->SR = 1u;
    TIM2->SR = 2u;
}

static int parse_cmd(const char *s)
{
    if (strcmp(s, "a") == 0) { return 1; }
    if (strcmp(s, "b") == 0) { return 2; }
    if (strcmp(s, "c") == 0) { return 3; }
    return 0;
}

static int pick(int x)
{
    switch (x) {
    case 1: return 1;
    case 2: return 2;
    case 3: return 3;
    case 4: return 4;
    default: return 0;
    }
}

/* 反例：两次字符串比较未达阈值 */
static int parse_two(const char *s)
{
    if (strcmp(s, "x") == 0) { return 1; }
    if (strcmp(s, "y") == 0) { return 2; }
    return 0;
}

/* 反例：3 个 case 未达阈值 */
static int pick_three(int x)
{
    switch (x) {
    case 1: return 1;
    case 2: return 2;
    case 3: return 3;
    default: return 0;
    }
}

int main(void)
{
    hw_init();
    return parse_cmd("a") + pick(1);
}
