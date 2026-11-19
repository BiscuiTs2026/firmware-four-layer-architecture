void pm_logic_tick(void);
void pm_f1_gpio_write(int v);

/* 正例：pm_ 前缀的端口函数，按定义层判定也要报 APP 直调 BSP */
void app_run(void)
{
    pm_logic_tick();
    pm_f1_gpio_write(1);
}
