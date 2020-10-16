/*
 * freertos_I2C.h
 *
 *  Created on: Oct 15, 2020
 *      Author: eduar
 */

#ifndef FREERTOS_I2C_H_
#define FREERTOS_I2C_H_

#include "stdint.h"

typedef enum
{
  freertos_i2c_0,
  freertos_i2c_1,
  freertos_i2c_2,
  freertos_i2c_3
} freertos_i2c_number_t;

typedef enum
{
  freertos_i2c_portA,
  freertos_i2c_portB,
  freertos_i2c_portC,
  freertos_i2c_portD,
  freertos_i2c_portE
} freertos_i2c_port_t;

typedef enum
{
  freertos_i2c_sucess,
  freertos_i2c_fail
} freertos_i2c_flag_t;

typedef struct
{
	uint32_t baudrate;
	freertos_i2c_number_t i2c_number;
	freertos_i2c_port_t port;
	uint8_t scl_pin;
	uint8_t sda_pin;
	uint8_t pin_mux;
} freertos_i2c_config_t;

freertos_i2c_flag_t freertos_i2c_init(freertos_i2c_config_t config);

freertos_i2c_flag_t freertos_i2c_receive(uint8_t slave_adress, uint8_t command, uint8_t* data, uint8_t read_size);

freertos_i2c_flag_t freertos_i2c_send(uint8_t slave_address, uint8_t *data, uint8_t write_size);


#endif /* FREERTOS_I2C_H_ */
