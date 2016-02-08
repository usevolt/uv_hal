################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/hal_adc_controller.c \
../src/hal_can_controller.c \
../src/hal_debug.c \
../src/hal_gpio_controller.c \
../src/hal_iap_controller.c \
../src/hal_reset_controller.c \
../src/hal_stdout.c \
../src/hal_timer_controller.c \
../src/hal_uart_controller.c \
../src/hal_wdt_controller.c \
../src/printf.c \
../src/uw_can.c \
../src/uw_canopen.c \
../src/uw_filters.c \
../src/uw_utilities.c 

OBJS += \
./src/hal_adc_controller.o \
./src/hal_can_controller.o \
./src/hal_debug.o \
./src/hal_gpio_controller.o \
./src/hal_iap_controller.o \
./src/hal_reset_controller.o \
./src/hal_stdout.o \
./src/hal_timer_controller.o \
./src/hal_uart_controller.o \
./src/hal_wdt_controller.o \
./src/printf.o \
./src/uw_can.o \
./src/uw_canopen.o \
./src/uw_filters.o \
./src/uw_utilities.o 

C_DEPS += \
./src/hal_adc_controller.d \
./src/hal_can_controller.d \
./src/hal_debug.d \
./src/hal_gpio_controller.d \
./src/hal_iap_controller.d \
./src/hal_reset_controller.d \
./src/hal_stdout.d \
./src/hal_timer_controller.d \
./src/hal_uart_controller.d \
./src/hal_wdt_controller.d \
./src/printf.d \
./src/uw_can.d \
./src/uw_canopen.d \
./src/uw_filters.d \
./src/uw_utilities.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -D__CODE_RED -DCORE_M0 -D__LPC11XX__ -I"/home/usevolt/uw/UW_LPC11C14_HAL/inc" -I"/home/usevolt/uw/UW_LPC11C14_HAL/CMSIS_CORE_LPC11xx/inc" -Os -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m0 -mthumb -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


