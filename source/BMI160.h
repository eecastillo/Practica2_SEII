/*
 * BMI160.h
 *
 *  Created on: Oct 15, 2020
 *      Author: eduar
 */

#ifndef BMI160_H_
#define BMI160_H_

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "freertos_I2C.h"
#include "fsl_port.h"

#define BAUDRATE 115200
#define I2C_SCL_PIN 24
#define I2C_SDA_PIN 25
#define I2C_DELAY	10

// Accelerometer Registers
#define ACC_Z_H 0x17
#define ACC_Z_L 0x16
#define ACC_Y_H 0x15
#define ACC_Y_L 0x14
#define ACC_X_H 0x13
#define ACC_X_L 0x12
// Gyroscope Registers
#define GYR_Z_H 0x11
#define GYR_Z_L 0x10
#define GYR_Y_H 0x0F
#define GYR_Y_L 0x0E
#define GYR_X_H 0x0D
#define GYR_X_L 0x0C
#define STATUS_REG 0x1B //bit 7 data ready accelerometer, bit 6 drdy gyroscope
#define GYR_CONF_REG 0x42
#define ACC_CONG_REG 0x40
#define SLAVE_ADRESS 0x68
// For PMU normal mode ACC 0x11, for PMU normal mode gyro 0x15
#define CMD_REGISTER 0x7E
#define ACC_NORMAL_MODE 0x11
#define GYR_NORMAL_MODE 0x15
//Chip ID code
#define CHIP_ID_REG 0x00
typedef struct
{
	int16_t x;
	int16_t y;
	int16_t z;
} bmi160_raw_data_t;

freertos_i2c_flag_t  BMI160_init(void);
bmi160_raw_data_t get_accelerometer(void);
bmi160_raw_data_t get_giroscope(void);

#endif /* BMI160_H_ */
