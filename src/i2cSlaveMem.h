/*
 * i2cSlaveMem.h
 *
 *  Created on: 8 Mar 2026
 *      Author: zuaobut
 */

#ifndef SRC_I2CSLAVEMEM_H_
#define SRC_I2CSLAVEMEM_H_
#include <stdint.h>
#include <stddef.h>

void i2cSlaveMem_init(uint8_t addr, uint8_t *mem, size_t memSize);

#endif /* SRC_I2CSLAVEMEM_H_ */
