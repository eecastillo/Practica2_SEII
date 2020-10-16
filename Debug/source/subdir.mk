################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/BMI160.c \
../source/freertos_I2C.c \
../source/freertos_uart.c \
../source/main.c \
../source/semihost_hardfault.c 

OBJS += \
./source/BMI160.o \
./source/freertos_I2C.o \
./source/freertos_uart.o \
./source/main.o \
./source/semihost_hardfault.o 

C_DEPS += \
./source/BMI160.d \
./source/freertos_I2C.d \
./source/freertos_uart.d \
./source/main.d \
./source/semihost_hardfault.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MK66FN2M0VMD18 -DCPU_MK66FN2M0VMD18_cm4 -DFSL_RTOS_FREE_RTOS -DSDK_OS_FREE_RTOS -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"D:\SEI2\Practica2\practica_2_gryroscope\board" -I"D:\SEI2\Practica2\practica_2_gryroscope\source" -I"D:\SEI2\Practica2\practica_2_gryroscope" -I"D:\SEI2\Practica2\practica_2_gryroscope\amazon-freertos\freertos_kernel\include" -I"D:\SEI2\Practica2\practica_2_gryroscope\amazon-freertos\freertos_kernel\portable\GCC\ARM_CM4F" -I"D:\SEI2\Practica2\practica_2_gryroscope\drivers" -I"D:\SEI2\Practica2\practica_2_gryroscope\device" -I"D:\SEI2\Practica2\practica_2_gryroscope\CMSIS" -I"D:\SEI2\Practica2\practica_2_gryroscope\component\uart" -I"D:\SEI2\Practica2\practica_2_gryroscope\utilities" -I"D:\SEI2\Practica2\practica_2_gryroscope\component\serial_manager" -I"D:\SEI2\Practica2\practica_2_gryroscope\component\lists" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


