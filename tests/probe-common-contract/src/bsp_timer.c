#include "res_contract.h"

/* 反例：契约符号被 bsp 重复定义（应只有 common 一份） */
int res_claim(int id)
{
    return id;
}

/* 反例：只由 bsp 实现的公共 API 头符号——不判（工程常把所有公共头映射成 common） */
int res_open(int id)
{
    return id;
}

/* 不算定义：函数指针赋值 */
static int (*res_hook)(int) = res_claim;

/* 不算定义：宏包装 */
#define res_wrap(x) res_claim(x)

/* 反例：跨行原型的契约符号同样被 bsp 重复定义 */
int
cmn_mul(int a, int b)
{
    return a * b;
}

int cmn_div(int a,
            int b)
{
    return b ? a / b : 0;
}

static const int
cmn_scale(int x)
{
    return x;
}
