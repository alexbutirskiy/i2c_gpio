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
      i2c->fn->setData(1); // Release data line
      ret = I2C_BUS_BUSY;  // Clock stretch timeout
    }
  }

  i2c->fn->delayUs(i2c->delayUs);

  return ret;
}

i2c_status_t i2c_readByte(i2c_t *i2c, uint8_t *data, ack_t ack)
{
  i2c_status_t ret = I2C_OK;
  for (int i = 0; i < 8 && ret == I2C_OK; i++)
  {
    ret = pulseClk(i2c, 1);
    *data >>= 1;
    *data |= (i2c->fn->getData() ? 0x80 : 0x00);
  }

  if (ret == I2C_OK)
  {
    // send ACK/NACK
    ret = pulseClk(i2c, ack == ACK ? 0 : 1);
  }

  return ret;
}

i2c_status_t i2c_writeByte(i2c_t *i2c, uint8_t data, ack_t *ack)
{
  i2c_status_t ret = I2C_OK;

  for (int i = 0; i < 8 && ret == I2C_OK; i++, data <<= 1)
  {
    ret = pulseClk(i2c, data & 0x80);
  }

  if (ret == I2C_OK)
  {
    // receive ACK/NACK
    ret = pulseClk(i2c, 1);
    *ack = i2c->fn->getData() ? NOACK : ACK;
  }

  return ret;
}

i2c_status_t i2c_start(i2c_t *i2c)
{
  i2c_status_t ret = I2C_OK;

  i2c->fn->setData(1);
  i2c->fn->delayUs(i2c->delayUs);

  if (i2c->fn->getData() == 0)
  {
    // data stuck, try to read 9 bits to release
    uint8_t tmp;
    ret = i2c_readByte(i2c, &tmp, NOACK);
    if (ret == I2C_OK && i2c->fn->getData() == 0)
    {
      ret = I2C_BUS_BUSY; // Could not release SDA line
    }
  }

  if (ret == I2C_OK)
  {
    i2c->fn->setData(0);
    i2c->fn->delayUs(i2c->delayUs);
  }

  return ret;
}

i2c_status_t i2c_stop(i2c_t *i2c)
{
  i2c_status_t ret = I2C_OK;

  // prepare data line for stop condition
  ret = pulseClk(i2c, 0);
  if (ret == I2C_OK)
  {
    i2c->fn->setData(1);
    i2c->fn->delayUs(i2c->delayUs);
  }

  return ret;
}

i2c_status_t i2c_restart(i2c_t *i2c)
{
  i2c_status_t ret = I2C_OK;

  // prepare data line for repeated start condition
  ret = pulseClk(i2c, 1);
  if (ret == I2C_OK)
  {
    i2c->fn->setData(0);
    i2c->fn->delayUs(i2c->delayUs);
  }

  return ret;
}

i2c_status_t i2c_read(i2c_t *i2c, uint8_t *data, size_t len)
{
  i2c_status_t ret = I2C_OK;
  for (size_t i = 0; i < len && ret == I2C_OK; i++)
  {
    ret = i2c_readByte(i2c, &data[i], i == len - 1 ? NOACK : ACK);
  }
  return ret;
}

i2c_status_t i2c_write(i2c_t *i2c, const uint8_t *data, size_t len)
{
  i2c_status_t ret = I2C_OK;
  ack_t ack;
  for (size_t i = 0; i < len && ret == I2C_OK; i++)
  {
    ret = i2c_writeByte(i2c, data[i], &ack);
    if (ret == I2C_OK && ack != ACK) // preserve ret if it's not I2C_OK
    {
      ret = I2C_NO_RESPONCE; // NACK received
      break;
    }
  }
  return ret;
}

i2c_status_t i2c_init(i2c_t *i2c, const i2c_fn_t *fn, uint16_t speedKHz)
{
  i2c_status_t ret = I2C_OK;
  if (i2c && fn)
  {
    i2c->fn = fn;
    i2c->delayUs = (1000 / speedKHz) / 2;
    i2c->fn->setData(1);
    i2c->fn->setClk(1);
  }
  else
  {
    ret = I2C_ERROR;
  }
  return ret;
}
