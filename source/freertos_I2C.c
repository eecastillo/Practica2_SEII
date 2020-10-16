/*
 * freertos_I2C.c
 *
 *  Created on: Oct 15, 2020
 *      Author: eduar
 */

#include "freertos_I2C.h"
#include "fsl_i2c.h"
#include "fsl_clock.h"
#include "fsl_port.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define NUMBER_OF_SERIAL_PORTS 4

typedef struct
{
	uint8_t is_init;
	i2c_master_handle_t i2c_master_handle;
	SemaphoreHandle_t mutex_sem;
	SemaphoreHandle_t binary_sem;
} freertos_i2c_hanlde_t;

static freertos_i2c_hanlde_t freertos_i2c_handles[NUMBER_OF_SERIAL_PORTS] = {0};

static inline void freertos_i2c_enable_port_clock(freertos_i2c_port_t port);

static inline void freertos_i2c_enable_clock(freertos_i2c_number_t i2c_number);

static inline PORT_Type * freertos_i2c_get_port_base(freertos_i2c_port_t port);

static inline I2C_Type * freertos_i2c_get_i2c_base(freertos_i2c_number_t i2c_number);

static void fsl_i2c_callback(I2C_Type *base, freertos_i2c_hanlde_t *handle, status_t status, void *userData);

freertos_i2c_flag_t freertos_i2c_init(freertos_i2c_config_t config)
{


	freertos_i2c_flag_t retval = freertos_i2c_fail;
	i2c_master_config_t fsl_i2c_config;

	const port_pin_config_t config_i2c =
	{ kPORT_PullUp, kPORT_SlowSlewRate, kPORT_PassiveFilterDisable,
			kPORT_OpenDrainDisable, kPORT_LowDriveStrength, kPORT_MuxAlt5,
			kPORT_UnlockRegister};

	if(config.i2c_number < NUMBER_OF_SERIAL_PORTS)
	{
		if(!freertos_i2c_handles[config.i2c_number].is_init)
		{
			freertos_i2c_handles[config.i2c_number].is_init = 0;
			freertos_i2c_handles[config.i2c_number].mutex_sem = xSemaphoreCreateMutex();
			freertos_i2c_handles[config.i2c_number].binary_sem = xSemaphoreCreateBinary();

		  /* Clock Enable */
			freertos_i2c_enable_port_clock(config.port);
			freertos_i2c_enable_clock(config.i2c_number);
			//PORT_SetPinMux(PORTE, config.scl_pin, config.pin_mux);
			//PORT_SetPinMux(PORTE, config.sda_pin, config.pin_mux);

			/* Port Config */
			PORT_SetPinConfig(freertos_i2c_get_port_base(config.port), config.sda_pin, &config_i2c);
			PORT_SetPinConfig(freertos_i2c_get_port_base(config.port), config.scl_pin, &config_i2c);

			I2C_MasterGetDefaultConfig(&fsl_i2c_config);
			//fsl_i2c_config.baudRate_Bps = config.baudrate;

			switch(config.i2c_number)
			{
				case freertos_i2c_0:
					I2C_MasterInit(freertos_i2c_get_i2c_base(freertos_i2c_0),&fsl_i2c_config, CLOCK_GetFreq(I2C0_CLK_SRC));
					NVIC_SetPriority(I2C0_IRQn, 5);
					break;
				case freertos_i2c_1:
					I2C_MasterInit(freertos_i2c_get_i2c_base(freertos_i2c_1),&fsl_i2c_config, CLOCK_GetFreq(I2C1_CLK_SRC));
					NVIC_SetPriority(I2C1_IRQn, 5);
					break;
				case freertos_i2c_2:
					I2C_MasterInit(freertos_i2c_get_i2c_base(freertos_i2c_2),&fsl_i2c_config, CLOCK_GetFreq(I2C2_CLK_SRC));
					NVIC_SetPriority(I2C2_IRQn, 5);
					break;
				case freertos_i2c_3:
					I2C_MasterInit(freertos_i2c_get_i2c_base(freertos_i2c_3),&fsl_i2c_config, CLOCK_GetFreq(I2C3_CLK_SRC));
					NVIC_SetPriority(I2C3_IRQn, 5);
					break;
			}

			I2C_MasterTransferCreateHandle(freertos_i2c_get_i2c_base(config.i2c_number), &freertos_i2c_handles[config.i2c_number].i2c_master_handle, fsl_i2c_callback, NULL);

			freertos_i2c_handles[config.i2c_number].is_init = 1;

			retval = freertos_i2c_sucess;
		}
	}

	return retval;
}

freertos_i2c_flag_t freertos_i2c_send(uint8_t slave_address, uint8_t *data, uint8_t write_size)
{
	freertos_i2c_flag_t flag = freertos_i2c_fail;
	i2c_master_transfer_t  master_xfer;

	if(freertos_i2c_handles[freertos_i2c_0].is_init)
	{
		master_xfer.slaveAddress = slave_address;
		master_xfer.direction = kI2C_Write;
		master_xfer.subaddressSize = 0;
		master_xfer.data = data;
		master_xfer.dataSize = write_size;
		master_xfer.flags = kI2C_TransferDefaultFlag;

		xSemaphoreTake(freertos_i2c_handles[freertos_i2c_0].mutex_sem, portMAX_DELAY);
		I2C_MasterTransferNonBlocking(I2C0, &freertos_i2c_handles[freertos_i2c_0].i2c_master_handle, &master_xfer);
		xSemaphoreTake(freertos_i2c_handles[freertos_i2c_0].binary_sem, portMAX_DELAY);
		xSemaphoreGive(freertos_i2c_handles[freertos_i2c_0].mutex_sem);

		flag = freertos_i2c_sucess;
	}

	return flag;
}

freertos_i2c_flag_t freertos_i2c_receive(uint8_t slave_adress, uint8_t command, uint8_t* data, uint8_t read_size)
{
  freertos_i2c_flag_t flag = freertos_i2c_fail;
  i2c_master_transfer_t master_xfer;
	i2c_master_transfer_t read_xfer;

	if(freertos_i2c_handles[freertos_i2c_0].is_init)
	{
		master_xfer.flags = kI2C_TransferNoStopFlag;
		master_xfer.slaveAddress = slave_adress;
		master_xfer.direction = kI2C_Write;
		master_xfer.subaddressSize = 0;
		master_xfer.data = &command;
		master_xfer.dataSize = 1;

		xSemaphoreTake(freertos_i2c_handles[freertos_i2c_0].mutex_sem, portMAX_DELAY);
		I2C_MasterTransferNonBlocking(I2C0, &freertos_i2c_handles[freertos_i2c_0].i2c_master_handle, &master_xfer);
		xSemaphoreTake(freertos_i2c_handles[freertos_i2c_0].binary_sem, portMAX_DELAY);

		read_xfer.slaveAddress = slave_adress;
		read_xfer.direction = kI2C_Read;
		read_xfer.subaddressSize = 0;
		read_xfer.data = data;
		read_xfer.dataSize = read_size;
		read_xfer.flags = kI2C_TransferRepeatedStartFlag;

		I2C_MasterTransferNonBlocking(I2C0, &freertos_i2c_handles[freertos_i2c_0].i2c_master_handle, &read_xfer);
		xSemaphoreTake(freertos_i2c_handles[freertos_i2c_0].binary_sem, portMAX_DELAY);
		xSemaphoreGive(freertos_i2c_handles[freertos_i2c_0].mutex_sem);

		flag = freertos_i2c_sucess;
	}

	return flag;
}

static inline void freertos_i2c_enable_port_clock(freertos_i2c_port_t port)
{
	switch(port)
	{
    case freertos_i2c_portA:
      CLOCK_EnableClock(kCLOCK_PortA);
      break;
    case freertos_i2c_portB:
      CLOCK_EnableClock(kCLOCK_PortB);
      break;
    case freertos_i2c_portC:
      CLOCK_EnableClock(kCLOCK_PortC);
      break;
    case freertos_i2c_portD:
      CLOCK_EnableClock(kCLOCK_PortD);
      break;
    case freertos_i2c_portE:
      CLOCK_EnableClock(kCLOCK_PortE);
      break;
	}
}

static inline void freertos_i2c_enable_clock(freertos_i2c_number_t i2c_number)
{
	switch(i2c_number)
	{
	case freertos_i2c_0:
		CLOCK_EnableClock(kCLOCK_I2c0);
		break;
	case freertos_i2c_1:
		CLOCK_EnableClock(kCLOCK_I2c1);
		break;
	case freertos_i2c_2:
		CLOCK_EnableClock(kCLOCK_I2c2);
		break;
	case freertos_i2c_3:
		CLOCK_EnableClock(kCLOCK_I2c3);
		break;
	}
}


static inline PORT_Type * freertos_i2c_get_port_base(freertos_i2c_port_t port)
{
  PORT_Type * port_base = PORTA;

  switch(port)
  {
    case freertos_i2c_portA:
      port_base = PORTA;
      break;
    case freertos_i2c_portB:
      port_base = PORTB;
      break;
    case freertos_i2c_portC:
      port_base = PORTC;
      break;
    case freertos_i2c_portD:
      port_base = PORTD;
      break;
    case freertos_i2c_portE:
      port_base = PORTE;
      break;
  }
  return port_base;
}

static inline I2C_Type * freertos_i2c_get_i2c_base(freertos_i2c_number_t i2c_number)
{
	I2C_Type * retval = I2C0;

	switch(i2c_number)
	{
	case freertos_i2c_0:
		retval = I2C0;
		break;
	case freertos_i2c_1:
		retval = I2C1;
		break;
	case freertos_i2c_2:
		retval = I2C2;
		break;
	case freertos_i2c_3:
		retval = I2C3;
		break;
	}

	return retval;
}

static void fsl_i2c_callback(I2C_Type *base, freertos_i2c_hanlde_t *handle, status_t status, void *userData)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (kStatus_Success  == status)
  {
	if(I2C0 == base)
	{
	    xSemaphoreGiveFromISR(freertos_i2c_handles[freertos_i2c_0].binary_sem, &xHigherPriorityTaskWoken);
	}
	else if(I2C1 == base)
	{
		xSemaphoreGiveFromISR(freertos_i2c_handles[freertos_i2c_1].binary_sem, &xHigherPriorityTaskWoken);
	}
	else if(I2C2 == base)
	{
		xSemaphoreGiveFromISR(freertos_i2c_handles[freertos_i2c_2].binary_sem, &xHigherPriorityTaskWoken);
	}
	else if(I2C3 == base)
	{
	    xSemaphoreGiveFromISR(freertos_i2c_handles[freertos_i2c_3].binary_sem, &xHigherPriorityTaskWoken);
	}
  }
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
