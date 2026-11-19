#include "res_contract.h"

int app_run(int id)
{
    res_claim(id);
    res_release(id);
    return 0;
}
