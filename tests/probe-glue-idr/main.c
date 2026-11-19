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
