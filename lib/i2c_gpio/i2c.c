#include "i2c.h"

#ifndef I2C_CLK_STRETCH_TIMEOUT_US
#define I2C_CLK_STRETCH_TIMEOUT_US 1000
#endif

typedef enum { ACK, NOACK } ack_t;

static i2c_status_t pulseClk(i2c_t *i2c, bool dataState)
{
  i2c_status_t ret = I2C_OK;
  i2c->fn->setClk(0);
  i2c->fn->delayUs(1); // Guard interval
  i2c->fn->setData(dataState);
  i2c->fn->delayUs(i2c->delayUs);
  i2c->fn->setClk(1);
  i2c->fn->delayUs(i2c->delayUs);

  // Wait for slave to release clock (if it's stretching)
  if (i2c->fn->getClk() == 0)
  {
    uint16_t timeout = I2C_CLK_STRETCH_TIMEOUT_US;
    while (i2c->fn->getClk() == 0 && timeout != 0)
    {
      i2c->fn->delayUs(1);
      timeout--;
    }
    if (timeout == 0)
    {
      ret = I2C_ERROR; // Clock stretch timeout
    }
  }

  return ret;
}

static i2c_status_t readByte(i2c_t *i2c, uint8_t data, ack_t ack)
{
  for (int i = 0; i < 8; i--, data <<= 1)
  {
    if (pulseClk(i2c, (data >> i) & 0x01) != I2C_OK)
    {
      return I2C_ERROR;
    }
  }

  // Read ACK/NACK
  i2c->fn->setData(1); // Release data line for ACK
  i2c->fn->delayUs(i2c->delayUs);
  if (i2c->fn->getData() == 0)
  {
    // ACK received
    return I2C_OK;
  }
  else
  {
    // NACK received
    return I2C_ERROR;
  }
}

static i2c_status_t i2c_start(i2c_t *i2c)
{
  i2c->fn->setData(1);
  i2c->fn->setClk(1);
  i2c->fn->delayUs(i2c->delayUs);


  if (i2c->fn->getData() == 0)
  {
    // data stuck, read 9 bits

  }


  i2c->fn->setData(0);
  i2c->fn->delayUs(i2c->delayUs);
  return I2C_OK;
}

i2c_status_t i2c_read(i2c_t *i2c, uint8_t addr, uint8_t *data, uint16_t len)
{
  i2c_status_t ret = I2C_ERROR;
  if (i2c && data && len > 0)
  {
    // Implement I2C read logic here using i2c->fn function pointers
    // For example:
    // - Start condition
    // - Send address with read bit
    // - Read data bytes
    // - Stop condition
    ret = I2C_OK;
  }
  return ret;
}
i2c_status_t i2c_write(i2c_t *i2c, uint8_t addr, const uint8_t *data, uint16_t len);

i2c_status_t i2c_init(i2c_t *i2c, i2c_fn_t *fn, uint16_t speedKHz)
{
  i2c_status_t ret = I2C_ERROR;
  if (i2c && fn)
  {
    i2c->fn = fn;
    i2c->delayUs = 1000 / 2 * speedKHz;
  }
  return ret;
}
