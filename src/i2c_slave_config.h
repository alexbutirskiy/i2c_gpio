/*
 * i2c_slave_config.h
 *
 *  Created on: Feb 17, 2026
 *      Author: butir
 */

#ifndef SRC_I2C_SLAVE_CONFIG_H_
#define SRC_I2C_SLAVE_CONFIG_H_
#include "stm32g0xx.h"
#define i2c_slave_int EXTI4_15_IRQHandler

#define I2C_SLAVE_SDA_PORT GPIOA
#define I2C_SLAVE_SDA_PIN  11

#define I2C_SLAVE_SCL_PORT GPIOA
#define I2C_SLAVE_SCL_PIN  12



#endif /* SRC_I2C_SLAVE_CONFIG_H_ */
