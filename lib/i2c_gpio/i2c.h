#ifndef I2C_GPIO_H
#define I2C_GPIO_H
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  I2C_OK = 0,
  I2C_ERROR = -1
} i2c_status_t;

typedef struct i2c_fn_s
{
  void (*setClk)(bool state);
  void (*setData)(bool state);
  bool (*getClk)(void);
  bool (*getData)(void);
  void (*delayUs)(uint16_t us);
} i2c_fn_t;

typedef struct i2c_s
{
  i2c_fn_t *fn;
  uint16_t delayUs;
} i2c_t;

i2c_status_t i2c_read(i2c_t *i2c, uint8_t addr, uint8_t *data, uint16_t len);
i2c_status_t i2c_write(i2c_t *i2c, uint8_t addr, const uint8_t *data, uint16_t len);

i2c_status_t i2c_init(i2c_t *i2c, i2c_fn_t *fn, uint16_t speedKHz);

#endif // I2C_GPIO_H
