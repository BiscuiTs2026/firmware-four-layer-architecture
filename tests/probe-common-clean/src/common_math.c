#include "common_math.h"

int32_t common_scale(int32_t v)
{
    return v * 2;
}

int32_t common_clamp(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}
