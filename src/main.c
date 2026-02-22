#include "stm32g0xx.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_hal_rcc.h"
#include "i2c.h"
#include "i2cDev.h"
#include "timer.h"
#include "i2c_slave.h"
#include "stm32g0xx_ll_exti.h"

static void SystemClock_Config(void)
{
    FLASH->ACR |= FLASH_ACR_LATENCY_2;

    RCC->PLLCFGR =
          RCC_PLLCFGR_PLLSRC_HSI
        | (8 << RCC_PLLCFGR_PLLN_Pos)   // x8
        | (1 << RCC_PLLCFGR_PLLR_Pos)   // /2 (there is no /1 option)
        | RCC_PLLCFGR_PLLREN;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    RCC->CFGR |= RCC_CFGR_SW_PLLRCLK;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLLRCLK);

    SystemCoreClockUpdate();
}

static void initI2cPin(GPIO_TypeDef *GPIOx, uint32_t pin)
{
  LL_GPIO_SetOutputPin(GPIOx, pin);
  LL_GPIO_SetPinMode(GPIOx, pin, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinSpeed(GPIOx, pin, LL_GPIO_SPEED_FREQ_MEDIUM);
  LL_GPIO_SetPinOutputType(GPIOx, pin, LL_GPIO_OUTPUT_OPENDRAIN);
  LL_GPIO_SetPinPull(GPIOx, pin, LL_GPIO_PULL_UP);
}

#define SDA GPIOA, LL_GPIO_PIN_4
#define SCL GPIOA, LL_GPIO_PIN_5


static void setData(bool state) {state ? LL_GPIO_SetOutputPin(SDA) : LL_GPIO_ResetOutputPin(SDA);}
static bool getData() {return LL_GPIO_IsInputPinSet(SDA);}
static void setClk(bool state) {state ? LL_GPIO_SetOutputPin(SCL) : LL_GPIO_ResetOutputPin(SCL);}
static bool getClk() {return LL_GPIO_IsInputPinSet(SCL);}
static void delayUs(uint16_t us)
{
  for (volatile uint16_t i = 0; i < us*2; i++);
}

i2c_status_t sc16is7x0_writeReg(i2cDev_t *dev, uint8_t regAddr, uint8_t value)
{
  return i2cDev_writeReg(dev, regAddr << 3, value);
}

i2c_status_t sc16is7x0_readReg(i2cDev_t *dev, uint8_t regAddr, uint8_t *value)
{
  return i2cDev_readReg(dev, regAddr << 3, value);
}

int main(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();

  // MCO to SYSCLK on PA8
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_8, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetAFPin_8_15(GPIOA, LL_GPIO_PIN_8, LL_GPIO_AF_0);
  LL_RCC_ConfigMCO(RCC_MCOSOURCE_SYSCLK, RCC_MCODIV_1);

  // GPIOA7
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_7, LL_GPIO_SPEED_FREQ_MEDIUM);
  LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_7, LL_GPIO_OUTPUT_PUSHPULL);

  initI2cPin(SDA);
  initI2cPin(SCL);

  I2c_SLAVE_INIT_INT();

  SystemClock_Config();
  SysTick_Config(SystemCoreClock / 1000);

  static i2c_t i2c_0;
  static const i2c_fn_t i2c_0_fn =
  {
    .setClk = setClk,
    .setData = setData,
    .getClk = getClk,
    .getData = getData,
    .delayUs = delayUs,
  };

  i2c_init(&i2c_0, &i2c_0_fn, 100);

  static i2cDev_t sc16is750_0;
  i2cDev_init(&sc16is750_0, &i2c_0, 0x48, 10, I2C_DEV_ADDR8);

  uint8_t reg = 0;

  sc16is7x0_writeReg(&sc16is750_0, 0x02, 0x07);
  sc16is7x0_writeReg(&sc16is750_0, 0x03, 0x80); // Enable access to special registers
  sc16is7x0_readReg(&sc16is750_0, 0x03, &reg);
  sc16is7x0_writeReg(&sc16is750_0, 0x00, 0x08); // Set baud rate divisor to 115200 at 14.7456MHz
  sc16is7x0_writeReg(&sc16is750_0, 0x01, 0x00); // Set baud rate divisor to 115200 at 14.7456MHz
  sc16is7x0_writeReg(&sc16is750_0, 0x03, 0x03); // Disable access to special registers and config 8 bits, no parity, 1 stop bit
  sc16is7x0_writeReg(&sc16is750_0, 0x04, 0x00);
  sc16is7x0_writeReg(&sc16is750_0, 0x07, 0xAA);

  uint8_t buf[10];
  for (size_t i = 0; i < sizeof(buf); i++)
  {
    i2cDev_read(&sc16is750_0, i, &buf[i], 1);
  }

  uint32_t timeStamp = timer_getTickCount();
  while(1)
  {
//    LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_7);
//    LL_GPIO_TogglePin(SDA);
//    LL_GPIO_TogglePin(SCL);

    if (timer_getTickDiff(timeStamp) >= 1000)
    {
      timeStamp = timer_getTickCount();
      uint8_t toSend[4] = { 0xAA, 0xAA, 0x55, 0x55 };
      i2cDev_write(&sc16is750_0, 0x00, toSend, sizeof(toSend));

      uint8_t fifoLevel;
      sc16is7x0_readReg(&sc16is750_0, 0x08, &fifoLevel); // tx
      sc16is7x0_readReg(&sc16is750_0, 0x09, &fifoLevel); // rx
    }
  }
}
