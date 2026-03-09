/*
 * i2cSlaveMem.c
 *
 *  Created on: 8 Mar 2026
 *      Author: zuaobut
 */
#include "i2cSlaveMem.h"
#include "i2c_slave.h"

static uint8_t *slaveMem;
static size_t slaveMemSize;

// From the master's perspective
enum { IDLE, WRITE_ADDR, READ, WRITE } state;

static uint16_t memAddr;
static uint8_t addrBytes;

#define ADDR_SIZE 2

static void i2cSlaveMem_callback(i2c_slave_callbackData_t data)
{
  switch (data.event)
  {
    case I2C_SLAVE_EVENT_ADDR_MATCH:
    {
      addrBytes = 0;
      break;
    }
    case I2C_SLAVE_EVENT_RX:
    {
      if (addrBytes < ADDR_SIZE)
      {
        memAddr <<= 8;
        memAddr |= data.data;
        addrBytes++;
      }
      else
      {
        if (memAddr >= slaveMemSize)
        {
          memAddr %= slaveMemSize; // Wrap around if address is out of bounds
        }
        slaveMem[memAddr++] = data.data;
      }

      break;
    }
    case I2C_SLAVE_EVENT_TX:
    {
      if (state == READ)
      {
        i2c_slave_tx(slaveMem[memAddr]);
        if (++memAddr >= slaveMemSize)
        {
          memAddr = 0;
        }
      }
      break;
    }
    case I2C_SLAVE_EVENT_STOP:
    {
      state = IDLE;
      break;
    }

    case I2C_SLAVE_EVENT_ERROR:
    case I2C_SLAVE_EVENT_NONE:
    {
      state = IDLE;
      break;
    }
  }
}

void i2cSlaveMem_init(uint8_t addr, uint8_t *mem, size_t memSize)
{
  slaveMem = mem;
  slaveMemSize = memSize;
  i2c_slave_addDevice(addr, &i2cSlaveMem_callback);
}

