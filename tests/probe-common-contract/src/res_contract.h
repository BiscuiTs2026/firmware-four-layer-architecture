#ifndef RES_CONTRACT_H
#define RES_CONTRACT_H

int res_claim(int id);
void res_release(int id);
int res_open(int id);

/* 跨行原型：返回类型独占一行 */
int
cmn_mul(int a, int b);

/* 跨行原型：参数表换行 */
int cmn_div(int a,
            int b);

/* 跨行原型：static / const 前导，返回类型再分行 */
static const int
cmn_scale(int x);

#endif
