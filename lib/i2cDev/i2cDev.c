#include "i2cDev.h"
#include "timer.h"

#define RW_READ 1
#define RW_WRITE 0

i2c_status_t i2cDev_init(i2cDev_t *dev, i2c_t *i2c, uint8_t addr, uint8_t timeoutMs,
                         i2cDev_addrMode_t addrMode)
{
  i2c_status_t ret = I2C_OK;

  if (dev && i2c)
  {
    dev->i2c = i2c;
    dev->devAddr = addr;
    dev->timeoutMs = timeoutMs;
    dev->addrMode = addrMode;
  }
  else
  {
    ret = I2C_ERROR;
  }
  return ret;
}

static size_t makeAddrBuf(i2cDev_t *dev, uint16_t regAddr, uint8_t addrBuf[3], bool rw)
{
  size_t size;
  addrBuf[0] = dev->devAddr << 1 | rw;
  if (dev->addrMode == I2C_DEV_ADDR8)
  {
    addrBuf[1] = regAddr & 0xFF;
    size = 2;
  }
  else
  {
    addrBuf[1] = (regAddr >> 8) & 0xFF;
    addrBuf[2] = regAddr & 0xFF;
    size = 3;
  }
  return size;
}

i2c_status_t i2cDev_read(i2cDev_t *dev, uint16_t addr, uint8_t *data, size_t len)
{
  if (!dev || !data)
  {
    return I2C_ERROR;
  }

  i2c_status_t ret;
  uint8_t buf[3];
  uint8_t size = makeAddrBuf(dev, addr, buf, RW_WRITE);
  uint32_t startTime = timer_getTickCount();
  do
  {
    ret = i2c_start(dev->i2c);
    if (ret != I2C_OK)
      goto exit;
    ret = i2c_write(dev->i2c, buf, size);
    if (ret == I2C_NO_RESPONCE)
    {
      i2c_stop(dev->i2c);
    }
  } while (ret == I2C_NO_RESPONCE && timer_getTickDiff(startTime) < dev->timeoutMs);

  if (ret != I2C_OK)
    goto exit;


  ret = i2c_restart(dev->i2c);
  if (ret != I2C_OK)
    goto exit;


  buf[0] = dev->devAddr << 1 | RW_READ;
  ret = i2c_write(dev->i2c, buf, 1);
  if (ret != I2C_OK)
  {
    goto exit;
  }

  ret = i2c_read(dev->i2c, data, len);

  if (ret != I2C_OK)
  {
    goto exit;
  }

  exit:
  i2c_stop(dev->i2c);
  return ret;
}

i2c_status_t i2cDev_write(i2cDev_t *dev, uint16_t addr, const uint8_t *data, size_t len)
{
  if (!dev || !data)
  {
    return I2C_ERROR;
  }

  i2c_status_t ret = i2c_start(dev->i2c);
  if (ret != I2C_OK)
    goto exit;


  uint8_t buf[3];
  uint8_t size = makeAddrBuf(dev, addr, buf, RW_WRITE);
  uint32_t startTime = timer_getTickCount();
  do
  {
    ret = i2c_write(dev->i2c, buf, size);
  } while (ret == I2C_NO_RESPONCE && timer_getTickDiff(startTime) < dev->timeoutMs);

  if (ret != I2C_OK)
    goto exit;

  ret = i2c_write(dev->i2c, data, len);

  if (ret != I2C_OK)
    goto exit;

  exit:
  i2c_stop(dev->i2c);

  return ret;
}

i2c_status_t i2cDev_writeReg(i2cDev_t *dev, uint8_t regAddr, uint8_t value)
{
  return i2cDev_write(dev, regAddr, &value, 1);
}

i2c_status_t i2cDev_readReg(i2cDev_t *dev, uint8_t regAddr, uint8_t *value)
{
  return i2cDev_read(dev, regAddr, value, 1);
}
