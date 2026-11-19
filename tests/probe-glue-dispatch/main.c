#include <string.h>

static int eq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

static int dispatch(const char *cmd)
{
    if (eq(cmd, "HELP")) { return 1; }
    else if (eq(cmd, "STAT")) { return 2; }
    else if (eq(cmd, "I2C")) { return 3; }
    else if (eq(cmd, "AHT")) { return 4; }
    else if (eq(cmd, "DIST")) { return 5; }
    return 0;
}

int main(void)
{
    return dispatch("HELP");
}
