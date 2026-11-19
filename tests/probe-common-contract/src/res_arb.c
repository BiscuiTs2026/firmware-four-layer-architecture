#include "res_contract.h"

int res_claim(int id)
{
    return id < 0 ? -1 : 0;
}

void res_release(int id)
{
    (void)id;
}

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
    return x * 2;
}
