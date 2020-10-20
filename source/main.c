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

typedef struct
{
	uint32_t header; //0xAAAAAAAA
	float x;
	float y;
	float z;
} comm_msg_t;
comm_msg_t g_message;
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
	TickType_t xfactor = pdMS_TO_TICKS(10);
	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
	bmi160_raw_data_t gyr_data;
	bmi160_raw_data_t acc_data;
	MahonyAHRSEuler_t mahony_euler;
	for( ;; )
	{
		g_message.header = 0xAAAAAAAA;
		gyr_data = get_giroscope();
		acc_data = get_accelerometer();
		// normalizar datos
		// desviacion estandar
		mahony_euler = MahonyAHRSupdateIMU((float)gyr_data.x,(float) gyr_data.y,(float) gyr_data.z,(float)acc_data.x,(float) acc_data.y,(float) acc_data.z);
		//PRINTF("\rData from accelerometer:  X = %f  Y = %f  Z = %f \n", (float)acc_data.x, (float)acc_data.y, (float)acc_data.z );
		//PRINTF("\rData from gyroscope:  X = %f  Y = %f  Z = %f \n", (float)gyr_data.x, (float)gyr_data.y, (float)gyr_data.z );
		//PRINTF("\rRoll: %.2f	Pitch: %.2f	Yaw: %.2f\n",mahony_euler.roll, mahony_euler.pitch, mahony_euler.yaw);
		g_message.x += mahony_euler.roll;
		g_message.y += mahony_euler.pitch;
		g_message.z += mahony_euler.yaw;
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
	for( ;; )
	{
		g_message.x /= times;
		g_message.y /= times;
		g_message.z /= times;
		freertos_uart_send(freertos_uart0, (uint8_t *) &g_message, sizeof(g_message));
		g_message.x = 0;
		g_message.y = 0;
		g_message.z = 0;
		times = 0;
		vTaskDelayUntil( &xLastWakeTime, xfactor );
	}
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



