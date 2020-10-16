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
	TickType_t xfactor = pdMS_TO_TICKS(500);
	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
	bmi160_raw_data_t gyr_data;
	bmi160_raw_data_t acc_data;
	MahonyAHRSEuler_t mahony_euler;
	for( ;; )
	{
		gyr_data = get_giroscope();
		acc_data = get_accelerometer();
		//mahony
		mahony_euler = MahonyAHRSupdateIMU(gyr_data.x, gyr_data.y, gyr_data.z,acc_data.x, acc_data.y, acc_data.z);
		PRINTF("\rData from accelerometer:  X = %d  Y = %d  Z = %d \n", acc_data.x, acc_data.y, acc_data.z );
		PRINTF("\rData from gyroscope:  X = %d  Y = %d  Z = %d \n", gyr_data.x, gyr_data.y, gyr_data.z );
		PRINTF("\rRoll: %.2f	Pitch: %.2f	Yaw: %.2f\n",mahony_euler.roll, mahony_euler.pitch, mahony_euler.yaw);
		vTaskDelayUntil( &xLastWakeTime, xfactor );
	}
}
void start_system(void *pvParameters)
{
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
	vTaskSuspend(NULL);
}



