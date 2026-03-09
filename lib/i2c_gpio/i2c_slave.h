#ifndef _I2C_SLAVE_H
#define _I2C_SLAVE_H
#include <stdint.h>
#include <stdbool.h>
#include "i2c_slave_config.h"

void I2c_SLAVE_INIT_INT(void);

typedef enum {I2C_ACK, I2C_NACK} i2c_ack_t;

typedef enum
{
  I2C_SLAVE_EVENT_NONE,
  I2C_SLAVE_EVENT_ADDR_MATCH,
  I2C_SLAVE_EVENT_RX,
  I2C_SLAVE_EVENT_TX,
  I2C_SLAVE_EVENT_STOP,

  I2C_SLAVE_EVENT_ERROR,
} i2c_slave_event_t;

typedef struct
{
  i2c_slave_event_t event;
  union
  {
    uint8_t data; // valid for RX and TX events
    struct
    {
      unsigned addr : 7;
      unsigned read : 1;
    };
  };
  bool ack;
} i2c_slave_callbackData_t;

typedef void (*i2c_slave_callback_t)(i2c_slave_callbackData_t data);

/**
 * @brief Register a slave device
 * @param addr 7-bit I2C address (bits 7-1, LSB is always 0)
 * @param cb
 */
void i2c_slave_addDevice(uint8_t addr, i2c_slave_callback_t cb);
void i2c_slave_tx(uint8_t data);
void i2c_slave_ack(i2c_ack_t ack);

#endif	// _I2C_SLAVE_H


