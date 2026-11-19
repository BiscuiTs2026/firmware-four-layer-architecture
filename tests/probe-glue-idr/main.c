#include "stm32f1xx.h"

/* 正例：胶水层直接读引脚做业务；反例：写 ODR 不判 */
int main(void)
{
    if (GPIOB->IDR & 0x1u) {
        return 1;
    }
    GPIOB->ODR = 0x1u;
    return 0;
}

/* 反例：ISR 内读 IDR 属中断分发（A1），G7 与 G1 同口径豁免 */
void EXTI15_10_IRQHandler(void)
{
    if (GPIOA->IDR & 0x2u) {
        GPIOA->BSRR = 0x2u;
    }
}
