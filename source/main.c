/*
 * Copyright 2016-2020 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/**
 * @file    Tarea6_i2c.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK66F18.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "BMI160.h"
#include "freertos_uart.h"
#include "mahony.h"
/* TODO: insert other include files here. */

/* TODO: insert other definitions and declarations here. */

/*
 * @brief   Application entry point.
 */


#define BMI160_init_PRIORITY (configMAX_PRIORITIES - 1)
#define BMI160_task_PRIORITY (configMAX_PRIORITIES - 2)



void get_readings(void *pvParameters);
void start_system(void *pvParameters);
void send_uart(void *pvParameters);
void calibrate_sensor(void *pvParameters);

typedef struct
{
	uint32_t header; //0xAAAAAAAA
	float x;
	float y;
	float z;
} comm_msg_t;
comm_msg_t g_message;
bmi160_raw_data_t g_calibrate_gyr;
bmi160_raw_data_t g_calibrate_acc;

bmi160_raw_data_t g_filter_gyr;
bmi160_raw_data_t g_filter_acc;
float times;

int main(void) {

  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();



    PRINTF("Hello World\n");

    if (xTaskCreate(start_system, "BMI160_init", configMINIMAL_STACK_SIZE + 100, NULL, BMI160_init_PRIORITY, NULL) !=
		pdPASS)
	{
		PRINTF("Failed to create task");
		while (1);
	}

	vTaskStartScheduler();
    for(;;){}
    return 0 ;
}

void get_readings(void *pvParameters)
{
	TickType_t xLastWakeTime;
	TickType_t xfactor = pdMS_TO_TICKS(5);
	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
	bmi160_raw_data_t gyr_data;
	bmi160_raw_data_t acc_data;

	g_filter_gyr.x = 0;
	g_filter_gyr.y = 0;
	g_filter_gyr.z = 0;

	g_filter_acc.x = 0;
	g_filter_acc.y = 0;
	g_filter_acc.z = 0;

	for( ;; )
	{
		gyr_data = get_giroscope();
		acc_data = get_accelerometer();
		// normalizar datos
		gyr_data.x -= g_calibrate_gyr.x;
		gyr_data.y -= g_calibrate_gyr.y;
		gyr_data.z -= g_calibrate_gyr.z;

		acc_data.x -= g_calibrate_acc.x;
		acc_data.y -= g_calibrate_acc.y;
		acc_data.z -= g_calibrate_acc.z;
		// desviacion estandar
		//PRINTF("\rData from accelerometer:  X = %f  Y = %f  Z = %f \n", (float)acc_data.x, (float)acc_data.y, (float)acc_data.z );
		//PRINTF("\rData from gyroscope:  X = %f  Y = %f  Z = %f \n", (float)gyr_data.x, (float)gyr_data.y, (float)gyr_data.z );
		//PRINTF("\rRoll: %.2f	Pitch: %.2f	Yaw: %.2f\n",mahony_euler.roll, mahony_euler.pitch, mahony_euler.yaw);
		g_filter_gyr.x += gyr_data.x;
		g_filter_gyr.y += gyr_data.y;
		g_filter_gyr.z += gyr_data.z;

		g_filter_acc.x += acc_data.x;
		g_filter_acc.y += acc_data.y;
		g_filter_acc.z += acc_data.z;

		times++;
		vTaskDelayUntil( &xLastWakeTime, xfactor );
	}
}
void send_uart(void *pvParameters)
{
	TickType_t xLastWakeTime;
	TickType_t xfactor = pdMS_TO_TICKS(20);
	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
	MahonyAHRSEuler_t mahony_euler;

	for( ;; )
	{
		g_message.header = 0xAAAAAAAA;

		g_filter_gyr.x /= times;
		g_filter_gyr.y /= times;
		g_filter_gyr.z /= times;

		g_filter_acc.x /= times;
		g_filter_acc.y /= times;
		g_filter_acc.z /= times;

		mahony_euler = MahonyAHRSupdateIMU((float)g_filter_gyr.x,(float) g_filter_gyr.y,(float) g_filter_gyr.z,(float)g_filter_acc.x,(float) g_filter_acc.y,(float) g_filter_acc.z);
		g_message.x = mahony_euler.roll;
		g_message.y = mahony_euler.pitch;
		g_message.z = mahony_euler.yaw;

		freertos_uart_send(freertos_uart0, (uint8_t *) &g_message, sizeof(g_message));
		g_filter_gyr.x = 0;
		g_filter_gyr.y = 0;
		g_filter_gyr.z = 0;

		g_filter_acc.x = 0;
		g_filter_acc.y = 0;
		g_filter_acc.z = 0;
		times = 0;

		vTaskDelayUntil( &xLastWakeTime, xfactor );
	}
}
void calibrate_sensor(void *pvParameters)
{
	TickType_t xLastWakeTime;
	TickType_t xfactor = pdMS_TO_TICKS(5);
	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
	g_calibrate_gyr.x=0;
	g_calibrate_gyr.y=0;
	g_calibrate_gyr.z=0;

	g_calibrate_acc.x=0;
	g_calibrate_acc.y=0;
	g_calibrate_acc.z=0;

	bmi160_raw_data_t acc_temp;
	bmi160_raw_data_t gyr_temp;

	for(uint8_t i = 0; i<100;i++)
	{
		gyr_temp = get_giroscope();
		acc_temp = get_accelerometer();

		g_calibrate_gyr.x += gyr_temp.x;
		g_calibrate_gyr.y += gyr_temp.y;
		g_calibrate_gyr.z += gyr_temp.z;

		g_calibrate_acc.x += acc_temp.x;
		g_calibrate_acc.y += acc_temp.y;
		g_calibrate_acc.z += acc_temp.z;

		vTaskDelayUntil( &xLastWakeTime, xfactor );
	}
	g_calibrate_gyr.x /= 100.0;
	g_calibrate_gyr.y /= 100.0;
	g_calibrate_gyr.z /= 100.0;

	g_calibrate_acc.x /= 100.0;
	g_calibrate_acc.y /= 100.0;
	g_calibrate_acc.z /= 100.0;
	if (xTaskCreate(get_readings, "BMI_160_read", configMINIMAL_STACK_SIZE + 100, NULL, BMI160_task_PRIORITY, NULL) !=
			pdPASS)
	{
		PRINTF("Failed to create task");
		while (1);
	}
	if (xTaskCreate(send_uart, "UART_send", configMINIMAL_STACK_SIZE + 100, NULL, BMI160_task_PRIORITY-1, NULL) !=
			pdPASS)
	{
		PRINTF("Failed to create task");
		while (1);
	}
	vTaskSuspend(NULL);
}


void start_system(void *pvParameters)
{
	times = 0;
	freertos_uart_config_t config;

	config.baudrate = 115200;
	config.rx_pin = 16;
	config.tx_pin = 17;
	config.pin_mux = kPORT_MuxAlt3;
	config.uart_number = freertos_uart0;
	config.port = freertos_uart_portB;
	freertos_uart_init(config);


	uint8_t sucess = freertos_i2c_sucess;
	sucess = BMI160_init();
	if(freertos_i2c_sucess == sucess)
	{
		PRINTF("BMI160 configured\n");
	}
	if (xTaskCreate(get_readings, "BMI_160_read", configMINIMAL_STACK_SIZE + 100, NULL, BMI160_task_PRIORITY, NULL) !=
		pdPASS)
	{
		PRINTF("Failed to create task");
		while (1);
	}
	if (xTaskCreate(send_uart, "UART_send", configMINIMAL_STACK_SIZE + 100, NULL, BMI160_task_PRIORITY-1, NULL) !=
			pdPASS)
	{
		PRINTF("Failed to create task");
		while (1);
	}
	vTaskSuspend(NULL);
}



