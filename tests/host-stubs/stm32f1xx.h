/*
 * 主机侧语法检查用的最小 CMSIS 设备头（stub）。
 *
 * 用途：没有 arm-none-eabi 工具链时，让 examples/ 能在 PC 上编译到 .obj，
 * 验证接口与 C 语法。它不模拟寄存器行为，也不能替代目标板构建。
 * 真正的固件构建请使用 STM32CubeF1 + arm-none-eabi-gcc。
 */
#ifndef HOST_STUB_STM32F1XX_H
#define HOST_STUB_STM32F1XX_H

#include <stdint.h>

typedef struct
{
  volatile uint32_t CRL;
  volatile uint32_t CRH;
  volatile uint32_t IDR;
  volatile uint32_t ODR;
  volatile uint32_t BSRR;
  volatile uint32_t BRR;
  volatile uint32_t LCKR;
} GPIO_TypeDef;

typedef struct
{
  volatile uint32_t CR;
  volatile uint32_t CFGR;
  volatile uint32_t CIR;
  volatile uint32_t APB2RSTR;
  volatile uint32_t APB1RSTR;
  volatile uint32_t AHBENR;
  volatile uint32_t APB2ENR;
  volatile uint32_t APB1ENR;
  volatile uint32_t BDCR;
  volatile uint32_t CSR;
} RCC_TypeDef;

typedef struct
{
  volatile uint32_t CTRL;
  volatile uint32_t LOAD;
  volatile uint32_t VAL;
  volatile uint32_t CALIB;
} SysTick_TypeDef;

#define GPIOA ((GPIO_TypeDef *)(uintptr_t)0x40010800UL)
#define GPIOB ((GPIO_TypeDef *)(uintptr_t)0x40010C00UL)
#define GPIOC ((GPIO_TypeDef *)(uintptr_t)0x40011000UL)

#define RCC ((RCC_TypeDef *)(uintptr_t)0x40021000UL)
#define SysTick ((SysTick_TypeDef *)(uintptr_t)0xE000E010UL)

#define RCC_APB2ENR_IOPBEN (1UL << 3U)
#define RCC_APB2ENR_IOPCEN (1UL << 4U)

#define GPIO_CRH_MODE13    (3UL << 20U)
#define GPIO_CRH_CNF13     (3UL << 22U)
#define GPIO_CRH_MODE13_1  (2UL << 20U)
#define GPIO_BSRR_BS13     (1UL << 13U)
#define GPIO_BSRR_BR13     (1UL << 29U)
#define GPIO_IDR_IDR13     (1UL << 13U)

extern uint32_t SystemCoreClock;

/* 返回值：0 = 成功，1 = 失败（与 CMSIS 一致） */
static inline uint32_t SysTick_Config(uint32_t ticks)
{
  if (ticks == 0U)
  {
    return 1U;
  }
  SysTick->LOAD = ticks - 1U;
  SysTick->VAL = 0U;
  SysTick->CTRL = 7U;
  return 0U;
}

#endif
