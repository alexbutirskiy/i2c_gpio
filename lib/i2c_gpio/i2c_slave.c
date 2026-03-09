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

static void SDA_IrqEnable()
{
  EXTI->RTSR1 |= (1u << I2C_SLAVE_SDA_PIN);
  EXTI->FTSR1 |= (1u << I2C_SLAVE_SDA_PIN);
  EXTI->IMR1  |= (1u << I2C_SLAVE_SDA_PIN);
}

__attribute__((unused)) static void SDA_IrqDisable()
{
  EXTI->IMR1  &= ~(1u << I2C_SLAVE_SDA_PIN);
}

__attribute__((unused)) static void SCL_IrqEnableBoth()
{
  EXTI->RTSR1 |= (1u << I2C_SLAVE_SCL_PIN);
  EXTI->FTSR1 |= (1u << I2C_SLAVE_SCL_PIN);
  EXTI->IMR1  |= (1u << I2C_SLAVE_SCL_PIN);
}

__attribute__((unused)) static void SCL_IrqEnableRise()
{
  EXTI->RTSR1 |= (1u << I2C_SLAVE_SCL_PIN);
  EXTI->FTSR1 &= ~(1u << I2C_SLAVE_SCL_PIN);
  EXTI->IMR1  |= (1u << I2C_SLAVE_SCL_PIN);
}

__attribute__((unused)) static void SCL_IrqEnableFail()
{
  EXTI->RTSR1 &= ~(1u << I2C_SLAVE_SCL_PIN);
  EXTI->FTSR1 |= (1u << I2C_SLAVE_SCL_PIN);
  EXTI->IMR1  |= (1u << I2C_SLAVE_SCL_PIN);
}

__attribute__((unused)) static void SCL_IrqDisable()
{
  EXTI->IMR1  &= ~(1u << I2C_SLAVE_SCL_PIN);
}

void I2c_SLAVE_INIT_INT(void)
{
  initPin(I2C_SLAVE_SDA_PORT, 1u << I2C_SLAVE_SDA_PIN);
  initPin(I2C_SLAVE_SCL_PORT, 1u << I2C_SLAVE_SCL_PIN);

  LL_EXTI_SetEXTISource(EXPAND_CAT(LL_EXTI_CONFIG_PORT, I2C_SLAVE_SDA_PORT_LETTER),
                        EXPAND_CAT(LL_EXTI_CONFIG_LINE, I2C_SLAVE_SDA_PIN));

  LL_EXTI_SetEXTISource(EXPAND_CAT(LL_EXTI_CONFIG_PORT, I2C_SLAVE_SCL_PORT_LETTER),
                        EXPAND_CAT(LL_EXTI_CONFIG_LINE, I2C_SLAVE_SCL_PIN));
  SCL_IrqEnableBoth();
  SDA_IrqEnable();

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


#define SDA_NOW_1	(1<<0)
#define SCL_NOW_1	(1<<1)
#define SDA_OLD_1	(1<<2)
#define SCL_OLD_1	(1<<3)
#define SDA_NOW_0	(0)
#define SCL_NOW_0	(0)
#define SDA_OLD_0	(0)
#define SCL_OLD_0	(0)

typedef enum
{
  START         = SCL_OLD_1 | SCL_NOW_1 | SDA_OLD_1 | SDA_NOW_0,
  STOP          = SCL_OLD_1 | SCL_NOW_1 | SDA_OLD_0 | SDA_NOW_1,

  CLOCK_DOWN_0  = SCL_OLD_1 | SCL_NOW_0 | SDA_OLD_0 | SDA_NOW_0,
  CLOCK_DOWN_1  = SCL_OLD_1 | SCL_NOW_0 | SDA_OLD_0 | SDA_NOW_1,
  CLOCK_DOWN_2  = SCL_OLD_1 | SCL_NOW_0 | SDA_OLD_1 | SDA_NOW_0,
  CLOCK_DOWN_3  = SCL_OLD_1 | SCL_NOW_0 | SDA_OLD_1 | SDA_NOW_1,

  CLOCK_UP_R0_0 = SCL_OLD_0 | SCL_NOW_1 | SDA_OLD_0 | SDA_NOW_0,
  CLOCK_UP_R0_1 = SCL_OLD_0 | SCL_NOW_1 | SDA_OLD_1 | SDA_NOW_0,
  CLOCK_UP_R1_0 = SCL_OLD_0 | SCL_NOW_1 | SDA_OLD_0 | SDA_NOW_1,
  CLOCK_UP_R1_1 = SCL_OLD_0 | SCL_NOW_1 | SDA_OLD_1 | SDA_NOW_1,

// Data changed when clock is low, nothing to do
  DATA_UP       = SCL_OLD_0 | SCL_NOW_0 | SDA_OLD_0 | SDA_NOW_1,
  DATA_DOWN     = SCL_OLD_0 | SCL_NOW_0 | SDA_OLD_1 | SDA_NOW_0,

// These states are erroneous because no change between old and now,
// It can happen if line returned to it's old state during interrupt enter
// (due to noise or interrupt latency)
  ERROR_1       = SCL_OLD_0 | SCL_NOW_0 | SDA_OLD_0 | SDA_NOW_0,
  ERROR_2       = SCL_OLD_0 | SCL_NOW_0 | SDA_OLD_1 | SDA_NOW_1,
  ERROR_3       = SCL_OLD_1 | SCL_NOW_1 | SDA_OLD_0 | SDA_NOW_0,
  ERROR_4       = SCL_OLD_1 | SCL_NOW_1 | SDA_OLD_1 | SDA_NOW_1,
}i2c_bus_state_t;

static i2c_bus_state_t state = 3;


static uint8_t slaveAddr;
static i2c_slave_callback_t eventCallback;

i2c_slave_callbackData_t evtData;

static uint8_t rx;
static uint8_t tx;
static bool ackTx;
static uint8_t bitCtr;

static i2c_slave_event_t evt;

// Read/Write from the master's perspective,
// OP_READ means master is reading from slave, so slave should transmit data
static enum { OP_IDLE, OP_ADDRESSING = 1, OP_READ = 2, OP_WRITE = 4 } operation;

static uint8_t opAddr;

static inline void setDataHighZ()
{
  I2C_SLAVE_SDA_PORT->BSRR = (1 << I2C_SLAVE_SDA_PIN);
}

static inline void setDataLow()
{
  I2C_SLAVE_SDA_PORT->BRR = (1 << I2C_SLAVE_SDA_PIN);
}

static inline void receiveBit(bool bit)
{
  if (operation == OP_ADDRESSING || operation == OP_WRITE)
  {
    if (bitCtr < 8)
    {
      rx = (rx << 1) | bit;
    }
    else
    {
      evt = I2C_SLAVE_EVENT_RX;
      bitCtr = 0;


      const i2c_slave_event_t evtTable[] =
      {
        [OP_IDLE] = I2C_SLAVE_EVENT_NONE,
        [OP_ADDRESSING] = I2C_SLAVE_EVENT_ADDR_MATCH,
        [OP_READ] = I2C_SLAVE_EVENT_TX,
        [OP_WRITE] = I2C_SLAVE_EVENT_RX,
      };

      evt = evtTable[operation];
      evtData.data = rx;
      evtData.ack = bit;

      if (operation == OP_ADDRESSING)
      {
        operation = (opAddr & 1) ? OP_READ : OP_WRITE;
      }
    }

    bitCtr++;

    if (bitCtr == 8 && operation == OP_ADDRESSING)
    {
      opAddr = rx;
      if ( (opAddr >> 1) == slaveAddr)
      {
        ackTx = I2C_ACK;
      }
      else
      {
        ackTx = I2C_NACK;
      }
    }
  }
}

static inline void transmitBit(void)
{
  bool txBit = 1;
  switch (operation)
  {
    case OP_ADDRESSING:
    {
      if (bitCtr == 8)
      {
        if (ackTx == I2C_ACK)
        {
          txBit = 0;
        }
        else
        {
          operation = OP_IDLE;
        }
        bitCtr = 0;
      }
      break;
    }
    case OP_WRITE:
    {
      if (bitCtr == 8 && ackTx == I2C_ACK)
      {
        txBit = 0;
        ackTx = I2C_NACK;
      }
      break;
    }

    case OP_READ:
    {
      if (bitCtr < 8)
      {
        txBit = tx & 0x80;
        tx <<= 1;
      }
      break;
    }

    default:
      break;
  }
  if (txBit)
  {
    setDataHighZ();
  }
  else
  {
    setDataLow();
  }
}

void i2c_slave_int()
{
  state = (state << 2) & 0xF; // copy state -> old
	state |= (I2C_SLAVE_SDA_PORT->IDR & (1u << I2C_SLAVE_SDA_PIN)) ? SDA_NOW_1 : 0;
	state |= (I2C_SLAVE_SCL_PORT->IDR & (1u << I2C_SLAVE_SCL_PIN)) ? SCL_NOW_1 : 0;

	evt = I2C_SLAVE_EVENT_NONE;

	switch(state)
	{
		case START:
		{
		  bitCtr=0;
		  operation = OP_ADDRESSING;
			break;
		}

		case STOP:
		{
			operation = OP_IDLE;
			break;
		}

		case CLOCK_UP_R0_0:
    case CLOCK_UP_R0_1:
    {
      receiveBit(0);
      break;
    }

	  case CLOCK_UP_R1_0:
	  case CLOCK_UP_R1_1:
	  {
	    receiveBit(1);
	    break;
	  }

		case CLOCK_DOWN_0:
		case CLOCK_DOWN_1:
		case CLOCK_DOWN_2:
		case CLOCK_DOWN_3:
		{
		  transmitBit();
		  break;
		}

		case DATA_UP:
		case DATA_DOWN:
    {

      break;
    }

		case ERROR_1:
    case ERROR_2:
	  case ERROR_3:
	  case ERROR_4:
    {
      operation = OP_IDLE;
      evt = I2C_SLAVE_EVENT_ERROR;
      break;
    }
  }

	if (evt != I2C_SLAVE_EVENT_NONE)
  {
    eventCallback((i2c_slave_callbackData_t){ .event = evt, .data = rx });
  }

  LL_EXTI_ClearRisingFlag_0_31(LL_EXTI_LINE_11 | LL_EXTI_LINE_12);
  LL_EXTI_ClearFallingFlag_0_31(LL_EXTI_LINE_11 | LL_EXTI_LINE_12);
}

void i2c_slave_addDevice(uint8_t addr, i2c_slave_callback_t cb)
{
  slaveAddr = addr;
  eventCallback = cb;
}

void i2c_slave_tx(uint8_t data)
{
  tx = data;
}

void i2c_slave_ack(i2c_ack_t a)
{
  ackTx = a;
}
