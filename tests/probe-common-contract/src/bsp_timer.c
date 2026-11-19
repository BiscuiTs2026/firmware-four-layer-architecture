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
