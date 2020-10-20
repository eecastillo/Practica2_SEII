

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


#define BMI160_init_PRIORITY (configMAX_PRIORITIES - 1)
#define BMI160_task_PRIORITY (configMAX_PRIORITIES - 2)
#define CALIBRATION_PRIORITY (configMAX_PRIORITIES - 2)


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
float g_times;

#define N_MUESTRAS 100
#define HEADER_HEX 0xAAAAAAAA
#define ZERO 	   0

int main(void) {

  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();

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

	g_filter_gyr.x = ZERO;
	g_filter_gyr.y = ZERO;
	g_filter_gyr.z = ZERO;

	g_filter_acc.x = ZERO;
	g_filter_acc.y = ZERO;
	g_filter_acc.z = ZERO;

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

		// añadir los datos al promedio
		g_filter_gyr.x += gyr_data.x;
		g_filter_gyr.y += gyr_data.y;
		g_filter_gyr.z += gyr_data.z;

		g_filter_acc.x += acc_data.x;
		g_filter_acc.y += acc_data.y;
		g_filter_acc.z += acc_data.z;

		g_times++;
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
		g_message.header = HEADER_HEX;

		g_filter_gyr.x /= g_times;
		g_filter_gyr.y /= g_times;
		g_filter_gyr.z /= g_times;

		g_filter_acc.x /= g_times;
		g_filter_acc.y /= g_times;
		g_filter_acc.z /= g_times;

		mahony_euler = MahonyAHRSupdateIMU((float)g_filter_gyr.x,(float) g_filter_gyr.y,(float) g_filter_gyr.z,(float)g_filter_acc.x,(float) g_filter_acc.y,(float) g_filter_acc.z);
		g_message.x = mahony_euler.roll;
		g_message.y = mahony_euler.pitch;
		g_message.z = mahony_euler.yaw;

		freertos_uart_send(freertos_uart0, (uint8_t *) &g_message, sizeof(g_message));
		g_filter_gyr.x = ZERO;
		g_filter_gyr.y = ZERO;
		g_filter_gyr.z = ZERO;

		g_filter_acc.x = ZERO;
		g_filter_acc.y = ZERO;
		g_filter_acc.z = ZERO;
		g_times = ZERO;

		vTaskDelayUntil( &xLastWakeTime, xfactor );
	}
}
void calibrate_sensor(void *pvParameters)
{
	TickType_t xLastWakeTime;
	TickType_t xfactor = pdMS_TO_TICKS(5);
	// Initialise the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
	g_calibrate_gyr.x=ZERO;
	g_calibrate_gyr.y=ZERO;
	g_calibrate_gyr.z=ZERO;

	g_calibrate_acc.x=ZERO;
	g_calibrate_acc.y=ZERO;
	g_calibrate_acc.z=ZERO;

	bmi160_raw_data_t acc_temp;
	bmi160_raw_data_t gyr_temp;

	for(uint8_t i = ZERO; i<N_MUESTRAS;i++)
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
	g_calibrate_gyr.x /= (N_MUESTRAS + 0.0);
	g_calibrate_gyr.y /= (N_MUESTRAS + 0.0);
	g_calibrate_gyr.z /= (N_MUESTRAS + 0.0);

	g_calibrate_acc.x /= (N_MUESTRAS + 0.0);
	g_calibrate_acc.y /= (N_MUESTRAS + 0.0);
	g_calibrate_acc.z /= (N_MUESTRAS + 0.0);
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
	g_times = ZERO;
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
	if (xTaskCreate(calibrate_sensor, "calibration_task", configMINIMAL_STACK_SIZE + 100, NULL, CALIBRATION_PRIORITY, NULL) !=
		pdPASS)
	{
		PRINTF("Failed to create task");
		while (1);
	}

	vTaskSuspend(NULL);
}



