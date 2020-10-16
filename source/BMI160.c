/*
 * BMI160.c
 *
 *  Created on: Oct 15, 2020
 *      Author: eduar
 */
#include "BMI160.h"

static freertos_i2c_config_t bmi160_config;

freertos_i2c_flag_t  BMI160_init(void)
{
	TickType_t  xLastWakeTime = xTaskGetTickCount();
	TickType_t   xfactor = pdMS_TO_TICKS(100);

	freertos_i2c_flag_t status = freertos_i2c_fail;
	bmi160_config.baudrate = BAUDRATE;
	bmi160_config.i2c_number = freertos_i2c_0;
	bmi160_config.pin_mux = kPORT_MuxAlt5;
	bmi160_config.port = freertos_i2c_portE;
	bmi160_config.scl_pin = I2C_SCL_PIN;
	bmi160_config.sda_pin = I2C_SDA_PIN;

	status = freertos_i2c_init(bmi160_config);



	if(freertos_i2c_sucess == status)
	{
		status = freertos_i2c_fail;
		uint8_t write[2] = {CMD_REGISTER, 0x00};

		write[1] = ACC_NORMAL_MODE;
		freertos_i2c_send(SLAVE_ADRESS, write, 2);
		vTaskDelayUntil(&xLastWakeTime,xfactor);

		write[1] = GYR_NORMAL_MODE;
		freertos_i2c_send(SLAVE_ADRESS, write, 2);
		vTaskDelayUntil(&xLastWakeTime,xfactor);

	}
	return status;
}
bmi160_raw_data_t get_accelerometer(void)
{

	bmi160_raw_data_t acc_data;
	uint8_t acc[6];
	uint16_t x = 0;
	uint16_t y = 0;
	uint16_t z = 0;

	freertos_i2c_receive(SLAVE_ADRESS, ACC_X_L, acc, 6);
	x = acc[0] | acc[1]<<8;
	y = acc[2] | acc[3]<<8;
	z = acc[4] | acc[5]<<8;

	acc_data.x = x;
	acc_data.y = y;
	acc_data.z = z;

	return acc_data;

}

bmi160_raw_data_t get_giroscope(void)
{
	bmi160_raw_data_t gyr_data;
	uint8_t gyr[6];
	uint16_t x = 0;
	uint16_t y = 0;
	uint16_t z = 0;

	freertos_i2c_receive(SLAVE_ADRESS, GYR_X_L, gyr, 6);
	x = gyr[0] | gyr[1]<<8;
	y = gyr[2] | gyr[3]<<8;
	z = gyr[4] | gyr[5]<<8;

	gyr_data.x = x;
	gyr_data.y = y;
	gyr_data.z = z;

	return gyr_data;	
}


