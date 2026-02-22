#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "i2c_slave.h"
#include "stm32g0xx_ll_exti.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_hal_rcc.h"




#define CAT(a, b) a##b
#define EXPAND_CAT(a, b) CAT(a, b)

#define GPIO(p) CAT(GPIO, p)

#define I2C_SLAVE_SDA_PORT GPIO(I2C_SLAVE_SDA_PORT_LETTER)
#define I2C_SLAVE_SCL_PORT GPIO(I2C_SLAVE_SCL_PORT_LETTER)


static void initPin(GPIO_TypeDef *GPIOx, uint32_t pin)
{
  LL_GPIO_SetOutputPin(GPIOx, pin);
  LL_GPIO_SetPinMode(GPIOx, pin, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinSpeed(GPIOx, pin, LL_GPIO_SPEED_FREQ_MEDIUM);
  LL_GPIO_SetPinOutputType(GPIOx, pin, LL_GPIO_OUTPUT_OPENDRAIN);
  LL_GPIO_SetPinPull(GPIOx, pin, LL_GPIO_PULL_UP);
}

void I2c_SLAVE_INIT_INT(void)
{
  initPin(I2C_SLAVE_SDA_PORT, 1u << I2C_SLAVE_SDA_PIN);
  initPin(I2C_SLAVE_SCL_PORT, 1u << I2C_SLAVE_SCL_PIN);

  LL_EXTI_SetEXTISource(EXPAND_CAT(LL_EXTI_CONFIG_PORT, I2C_SLAVE_SDA_PORT_LETTER),
                        EXPAND_CAT(LL_EXTI_CONFIG_LINE, I2C_SLAVE_SDA_PIN));
  LL_EXTI_SetEXTISource(EXPAND_CAT(LL_EXTI_CONFIG_PORT, I2C_SLAVE_SCL_PORT_LETTER),
                        EXPAND_CAT(LL_EXTI_CONFIG_LINE, I2C_SLAVE_SCL_PIN));
  EXTI->RTSR1 |= (1u << I2C_SLAVE_SCL_PIN);
  EXTI->FTSR1 |= (1u << I2C_SLAVE_SCL_PIN);
  EXTI->IMR1  |= (1u << I2C_SLAVE_SCL_PIN);

  if (I2C_SLAVE_SDA_PIN <= 1 || I2C_SLAVE_SCL_PIN <= 1)
  {
    NVIC_EnableIRQ(EXTI0_1_IRQn);
  }

  if (I2C_SLAVE_SDA_PIN == 2 || I2C_SLAVE_SCL_PIN == 2 || I2C_SLAVE_SDA_PIN == 3 || I2C_SLAVE_SCL_PIN == 3)
  {
    NVIC_EnableIRQ(EXTI2_3_IRQn);
  }

  if (I2C_SLAVE_SDA_PIN >= 4 || I2C_SLAVE_SCL_PIN >= 4)
  {
    NVIC_EnableIRQ(EXTI4_15_IRQn);
  }
}


#define I2C_SLAVE_SCL_IE(en) do {} while(0)
#define I2C_SLAVE_SDA_IE(en) do {} while(0)
#define I2C_SLAVE_IE(en) do \
{\
  I2C_SLAVE_SCL_IE(en); \
  I2C_SLAVE_SDA_IE(en); \
} while(0)





#define SDA_NOW_1	(1<<0)
#define SCL_NOW_1	(1<<1)
#define SDA_OLD_1	(1<<2)
#define SCL_OLD_1	(1<<3)
#define SDA_NOW_0	(0)
#define SCL_NOW_0	(0)
#define SDA_OLD_0	(0)
#define SCL_OLD_0	(0)


#define START         SCL_OLD_1 | SCL_NOW_1 | SDA_OLD_1 | SDA_NOW_0
#define STOP          SCL_OLD_1 | SCL_NOW_1 | SDA_OLD_0 | SDA_NOW_1
#define CLOCK_DOWN_1  SCL_OLD_1 | SCL_NOW_0 | SDA_OLD_1 | SDA_NOW_1
#define CLOCK_DOWN_2  SCL_OLD_1 | SCL_NOW_0 | SDA_OLD_0 | SDA_NOW_0

#define CLOCK_DOWN_3  SCL_OLD_1 | SCL_NOW_0 | SDA_OLD_1 | SDA_NOW_0  // not expected
#define CLOCK_DOWN_4  SCL_OLD_1 | SCL_NOW_0 | SDA_OLD_1 | SDA_NOW_1  // not expected

#define CLOCK_UP_0    SCL_OLD_0 | SCL_NOW_1 | SDA_OLD_0 | SDA_NOW_0
#define CLOCK_UP_0_1  SCL_OLD_0 | SCL_NOW_1 | SDA_OLD_1 | SDA_NOW_0  // not expected

#define CLOCK_UP_1    SCL_OLD_0 | SCL_NOW_1 | SDA_OLD_1 | SDA_NOW_1
#define CLOCK_UP_1_1  SCL_OLD_0 | SCL_NOW_1 | SDA_OLD_0 | SDA_NOW_1  // not expected

static uint8_t state = 3;
bool start;
bool restart;
bool stop;

static uint8_t rx;
static uint8_t tx;
static bool ack;
static bool nextTxBit;
static uint8_t bitCtr;
static bool transmitting;

void i2c_slave_int()
{
	state = (state << 2) & 0xF; // copy state -> old
	state |= (I2C_SLAVE_SDA_PORT->IDR & (1u << I2C_SLAVE_SDA_PIN)) ? SDA_NOW_1 : 0;
	state |= (I2C_SLAVE_SCL_PORT->IDR & (1u << I2C_SLAVE_SCL_PIN)) ? SCL_NOW_1 : 0;

	switch(state)
	{
		case START:
		{
		  if (stop)
		  {
		    stop = false;
		    start = true;

		  }
		  else
		  {
		    restart = true;
		  }
		  bitCtr=0;
		  nextTxBit = 1;
			break;
		}

		case STOP:
		{
			stop = true;
			nextTxBit = 1;
			break;
		}

		case CLOCK_DOWN_1:
		case CLOCK_DOWN_2:
		{
		  if (nextTxBit)
		  {
//		    I2C_SLAVE_SDA_PORT->BSRR = (1 << I2C_SLAVE_SDA_PIN);
		  }
		  else
		  {
//		    I2C_SLAVE_SDA_PORT->BRR = (1 << I2C_SLAVE_SDA_PIN);
		  }
		  break;
		}

		case CLOCK_UP_0:
		{
		  rx = (rx >> 1);
		  if (transmitting)
		  {
		    tx <<= 1;
		    nextTxBit = tx & 0x80;
		  }
		  break;
		}

    case CLOCK_UP_1:
    {
      rx = (rx >> 1) | 0x80;
      if (transmitting)
      {
        tx <<= 1;
        nextTxBit = tx & 0x80;
      }
      break;
    }

	}
	LL_EXTI_ClearRisingFlag_0_31(LL_EXTI_LINE_11);
	LL_EXTI_ClearFallingFlag_0_31(LL_EXTI_LINE_11);
}
