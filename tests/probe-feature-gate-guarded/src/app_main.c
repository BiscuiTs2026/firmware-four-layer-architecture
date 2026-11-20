#define DIST_TIME_BASE_READY 0

void pm_sonar_init(void);

int main(void)
{
#if DIST_TIME_BASE_READY
    pm_sonar_init();
#endif
    return 0;
}
