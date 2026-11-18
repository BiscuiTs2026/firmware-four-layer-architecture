#include "i2c_soft.h"

#include "board_config.h"
#include "bsp_time.h"
#include "stm32f1xx.h"

/* 软件 I2C：SCL=PB6，SDA=PB7，开漏输出 */
static void scl_high(void)
{
  GPIOB->BSRR = (1UL << 6U);
}

static void scl_low(void)
{
  GPIOB->BSRR = (1UL << 22U);
}

static void sda_high(void)
{
  GPIOB->BSRR = (1UL << 7U);
}

static void sda_low(void)
{
  GPIOB->BSRR = (1UL << 23U);
}

static uint8_t sda_read(void)
{
  return (uint8_t)((GPIOB->IDR & (1UL << 7U)) != 0U);
}

/* 器件手册要求的半周期延时约 5 us；BSP 自带微秒原语，不依赖 OSAL */
#define I2C_HALF_PERIOD_US 5U

err_t i2c_soft_init(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

  /* PB6/PB7：开漏输出 2 MHz */
  GPIOB->CRL = (GPIOB->CRL & ~((0xFUL << 24U) | (0xFUL << 28U)))
             | ((0x6UL << 24U) | (0x6UL << 28U));
  scl_high();
  sda_high();
  return ERR_OK;
}

static void i2c_start(void)
{
  sda_high();
  scl_high();
  bsp_delay_us(I2C_HALF_PERIOD_US);
  sda_low();
  bsp_delay_us(I2C_HALF_PERIOD_US);
  scl_low();
}

static void i2c_stop(void)
{
  sda_low();
  scl_high();
  bsp_delay_us(I2C_HALF_PERIOD_US);
  sda_high();
  bsp_delay_us(I2C_HALF_PERIOD_US);
}

static err_t i2c_write_byte(uint8_t byte)
{
  for (uint8_t i = 0U; i < 8U; i++)
  {
    if ((byte & 0x80U) != 0U)
    {
      sda_high();
    }
    else
    {
      sda_low();
    }
    bsp_delay_us(I2C_HALF_PERIOD_US);
    scl_high();
    bsp_delay_us(I2C_HALF_PERIOD_US);
    scl_low();
    byte = (uint8_t)(byte << 1U);
  }

  /* 第 9 拍读 ACK */
  sda_high();
  scl_high();
  bsp_delay_us(I2C_HALF_PERIOD_US);
  if (sda_read() != 0U)
  {
    scl_low();
    return ERR_IO;
  }
  scl_low();
  return ERR_OK;
}

err_t i2c_soft_probe(uint8_t addr7)
{
  err_t err;

  i2c_start();
  err = i2c_write_byte((uint8_t)(addr7 << 1U));
  i2c_stop();
  return err;
}

err_t i2c_soft_write(uint8_t addr7, const uint8_t *buf, uint16_t len)
{
  err_t err;

  if (buf == 0)
  {
    return ERR_NULL;
  }
  if (len == 0U)
  {
    return ERR_PARAM;
  }

  i2c_start();
  err = i2c_write_byte((uint8_t)(addr7 << 1U));
  for (uint16_t i = 0U; (err == ERR_OK) && (i < len); i++)
  {
    err = i2c_write_byte(buf[i]);
  }
  i2c_stop();
  return err;
}
