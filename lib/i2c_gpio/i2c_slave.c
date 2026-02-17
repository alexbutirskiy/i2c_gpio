#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "i2c_slave.h"
#include "i2c_slave_config.h"
//#pragma GCC optimize ("-O3")
//typedef struct
//{
//  unsigned sda 		:1;
//  unsigned scl 		:1;
//  unsigned sdaOld	:1;
//  unsigned sclOld	:1;
//} pinState_t;
//
//static pinState_t pinState;
//
//void i2c_slave_int()
//{
//	pinState.sdaOld = pinState.sda;
//	pinState.sclOld = pinState.scl;
//	pinState.sda = I2C_SLAVE_SDA_PORT->IDR & (1u << I2C_SLAVE_SDA_PIN);
//	pinState.scl = I2C_SLAVE_SCL_PORT->IDR & (1u << I2C_SLAVE_SCL_PIN);
//}

#define SDA_NOW	(1<<0)
#define SCL_NOW	(1<<1)
#define SDA_OLD	(1<<2)
#define SCL_OLD	(1<<3)

#define START         SCL_OLD |  SCL_NOW |  SDA_OLD | !SDA_NOW
#define STOP          SCL_OLD |  SCL_NOW | !SDA_OLD |  SDA_NOW
#define CLOCK_DOWN_1  SCL_OLD | !SCL_NOW |  SDA_OLD |  SDA_NOW
#define CLOCK_DOWN_2  SCL_OLD | !SCL_NOW | !SDA_OLD | !SDA_NOW

#define CLOCK_DOWN_3  SCL_OLD | !SCL_NOW | SDA_OLD | !SDA_NOW      // not expected
#define CLOCK_DOWN_4  SCL_OLD | !SCL_NOW | SDA_OLD |  SDA_NOW      // not expected

#define CLOCK_UP_0      !SCL_OLD | SCL_NOW |  !SDA_OLD | !SDA_NOW
#define CLOCK_UP_0_1    !SCL_OLD | SCL_NOW |   SDA_OLD | !SDA_NOW  // not expected

#define CLOCK_UP_1      !SCL_OLD | SCL_NOW |   SDA_OLD |  SDA_NOW
#define CLOCK_UP_1_1    !SCL_OLD | SCL_NOW |  !SDA_OLD |  SDA_NOW  // not expected

static uint8_t state;
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
	state <<= 2; // copy state -> old
	state |= (I2C_SLAVE_SDA_PORT->IDR & (1u << I2C_SLAVE_SDA_PIN)) ? SDA_NOW : 0;
	state |= (I2C_SLAVE_SCL_PORT->IDR & (1u << I2C_SLAVE_SCL_PIN)) ? SDA_NOW : 0;

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
		    I2C_SLAVE_SDA_PORT->BSRR = (1 << I2C_SLAVE_SDA_PIN);
		  }
		  else
		  {
		    I2C_SLAVE_SDA_PORT->BRR = (1 << I2C_SLAVE_SDA_PIN);
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
}
