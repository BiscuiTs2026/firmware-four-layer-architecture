#include <stdint.h>

/* 正例：决策型 if 金字塔 4 层（每层都带 else） */
static int decide(int a, int b, int c, int d)
{
    if (a) {
        if (b) {
            if (c) {
                if (d) {
                    return 1;
                } else {
                    return 2;
                }
            } else {
                return 3;
            }
        } else {
            return 4;
        }
    } else {
        return 5;
    }
}

/* 反例：4 层单分支守卫不算决策嵌套 */
static int guards(int a, int b, int c, int d)
{
    if (a) {
        if (b) {
            if (c) {
                if (d) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int main(void)
{
    return decide(1, 1, 1, 1) + guards(1, 1, 1, 1);
}
