#ifndef I2C_GPIO_H
#define I2C_GPIO_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
  I2C_OK,
  I2C_ERROR,
  I2C_BUS_BUSY,
  I2C_NO_RESPONCE,
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
  const i2c_fn_t *fn;
  uint16_t delayUs;
} i2c_t;

i2c_status_t i2c_start(i2c_t *i2c);
i2c_status_t i2c_stop(i2c_t *i2c);
i2c_status_t i2c_restart(i2c_t *i2c);

i2c_status_t i2c_read(i2c_t *i2c, uint8_t *data, size_t len);
i2c_status_t i2c_write(i2c_t *i2c, const uint8_t *data, size_t len);

i2c_status_t i2c_init(i2c_t *i2c, const i2c_fn_t *fn, uint16_t speedKHz);

#endif // I2C_GPIO_H
