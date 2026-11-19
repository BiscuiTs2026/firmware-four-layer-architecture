#include "res_contract.h"

int res_claim(int id)
{
    return id < 0 ? -1 : 0;
}

void res_release(int id)
{
    (void)id;
}
