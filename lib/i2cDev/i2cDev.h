#ifndef LIB_I2C_DEV_H_
#define LIB_I2C_DEV_H_
#include "i2c.h"

typedef enum { I2C_DEV_ADDR8, I2C_DEV_ADDR16 } i2cDev_addrMode_t;
typedef struct i2cDev_s
{
  i2c_t *i2c;
  uint8_t devAddr;
  uint8_t timeoutMs;
  i2cDev_addrMode_t addrMode;
} i2cDev_t;

i2c_status_t i2cDev_init(i2cDev_t *dev, i2c_t *i2c, uint8_t addr, uint8_t timeoutMs,
                         i2cDev_addrMode_t addrMode);

i2c_status_t i2cDev_read(i2cDev_t *dev, uint16_t addr, uint8_t *data, size_t len);
i2c_status_t i2cDev_write(i2cDev_t *dev, uint16_t addr, const uint8_t *data, size_t len);

i2c_status_t i2cDev_writeReg(i2cDev_t *dev, uint8_t regAddr, uint8_t value);
#endif /* LIB_I2C_DEV_H_ */
