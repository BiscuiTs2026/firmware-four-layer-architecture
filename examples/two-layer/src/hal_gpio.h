#ifndef HAL_GPIO_H
#define HAL_GPIO_H

/* 只有标准整型与语义化接口，上层看不到 GPIOA / GPIO_TypeDef */
int hal_gpio_init(void);
void hal_gpio_write(int level);

#endif
