/*
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "uv_rtos.h"

#include "uv_uart.h"
#include "uv_utilities.h"
#include "uv_memory.h"
#include "uv_lcd.h"
#include "uv_spi.h"
#include "uv_i2c.h"
#include "uv_pwm.h"
#include "uv_eeprom.h"
#include "uv_emc.h"
#include "uv_rtc.h"
#include "uv_ft81x.h"
#if CONFIG_WDT
#include "uv_wdt.h"
#endif
#if CONFIG_ADC || CONFIG_ADC0 || CONFIG_ADC1
#include "uv_adc.h"
#endif
#include "uv_dac.h"
#if CONFIG_TERMINAL_USBDVCOM
#include "cdc_vcom.h"
#endif
#include "uv_terminal.h"



void vAssertCalled( const char * const pcFileName,  unsigned long ulLine ) {
	uv_terminal_enable(TERMINAL_CAN);
	printf_flags(PRINTF_FLAGS_NOTXCALLB,
			"Assert in '%s', line %u\n",
			pcFileName,
			(unsigned int) ulLine);
	while(true);
}


/** configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 * used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside this
     * function then they must be declared static - otherwise they will be allocated on
     * the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}


/**
 * @brief This is to provide the memory that is used by the RTOS daemon/time task.
 *
 * If configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetTimerTaskMemory() to provide the memory that is
 * used by the RTOS daemon/time task.
 */
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize )
{
    /* If the buffers to be provided to the Timer task are declared inside this
     * function then they must be declared static - otherwise they will be allocated on
     * the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}



typedef struct {
	void (*idle_task)(void *user_ptr);
	void (*tick_task)(void *user_ptr, unsigned int step_ms);
} this_st;
bool rtos_init = false;

static volatile this_st _this = {
		.idle_task = NULL,
		.tick_task = NULL
};


uv_mutex_st halmutex;
uv_rtos_task_ptr hal_task_ptr;


uv_errors_e uv_queue_peek(uv_queue_st *this, void *dest, int32_t wait_ms) {
	uv_errors_e ret = ERR_NONE;
	if (xQueuePeek(*this, dest, wait_ms / portTICK_PERIOD_MS) == pdFALSE) {
		ret = ERR_BUFFER_EMPTY;
	}
	return ret;
}


uv_errors_e uv_queue_push(uv_queue_st *this, void *src, int32_t wait_ms) {
	uv_errors_e ret = ERR_NONE;
	if (xQueueSend(*this, src, wait_ms / portTICK_PERIOD_MS) == pdFALSE) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	return ret;
}


uv_errors_e uv_queue_pop(uv_queue_st *this, void *dest, int32_t wait_ms) {
	uv_errors_e ret = ERR_NONE;
	if (xQueueReceive(*this, dest, wait_ms / portTICK_PERIOD_MS) == pdFALSE) {
		ret = ERR_BUFFER_EMPTY;
	}
	return ret;
}


uv_errors_e uv_queue_pop_isr(uv_queue_st *this, void *dest) {
	uv_errors_e ret = ERR_NONE;

	if (!xQueueReceiveFromISR(*this, dest, NULL)) {
		ret = ERR_BUFFER_EMPTY;
	}

	return ret;
}


uv_errors_e uv_queue_push_isr(uv_queue_st *this, void *src) {
	uv_errors_e ret = ERR_NONE;
	if (!xQueueSendFromISR(*this, src, NULL)) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	return ret;
}








void uv_mutex_unlock_isr(uv_mutex_st *mutex) {
	BaseType_t woken;
	xSemaphoreGiveFromISR(*mutex, &woken);
	portEND_SWITCHING_ISR(woken);
}




#define this (&_this)


void uv_rtos_start_scheduler(void) {
	vTaskStartScheduler();
}


/// @brief: Task function which takes care of calling several hal librarys module
/// hal step functions
void hal_task(void *);

void canopen_task(void *);


int32_t uv_rtos_task_create(void (*task_function)(void *this_ptr), char *task_name,
		unsigned int stack_depth, void *this_ptr,
		unsigned int task_priority, uv_rtos_task_ptr* handle) {
	static unsigned int size = 0;
	size += stack_depth * 4;
	if (size >= CONFIG_RTOS_HEAP_SIZE) {
		while(true) {
			printf("Out of memory\r");
		}
	}
	int32_t ret = xTaskCreate(task_function, (const char * const)task_name, stack_depth,
			this_ptr, task_priority, handle);

	if (ret == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) {
		uv_terminal_enable(TERMINAL_CAN);
		printf("RTOS ERR: Could not allocate memory for task '%s'\n",
				task_name);
	}
	return ret;
}



uv_errors_e uv_rtos_add_idle_task(void (*task_function)(void *user_ptr)) {
	this->idle_task = task_function;
	return ERR_NONE;
}

void uv_rtos_task_delay(unsigned int ms) {
	portTickType xDelayTime;

	xDelayTime = xTaskGetTickCount();
	vTaskDelayUntil(&xDelayTime, ms);
}


void vApplicationMallocFailedHook(void)
{
	taskDISABLE_INTERRUPTS();
	for (;; ) {}
}

/* FreeRTOS application idle hook */
void vApplicationIdleHook(void)
{
	if (this->idle_task) {
		this->idle_task(__uv_get_user_ptr());

	}
}

/* FreeRTOS stack overflow hook */
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
	(void) pxTask;
	(void) pcTaskName;

	uv_terminal_enable(TERMINAL_CAN);
	printf_flags(PRINTF_FLAGS_NOTXCALLB,
			"stack overflow from task: %s\n", pcTaskName);
	/* Run time stack overflow checking is performed if
	   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	   function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();
	for (;; ) {}
}

/* FreeRTOS application tick hook */
void vApplicationTickHook(void) {
	if (this->tick_task) {
		this->tick_task(__uv_get_user_ptr(), portTICK_PERIOD_MS);
	}
}





void uv_init(void *device) {
	uv_set_application_ptr(device);
	uv_mutex_init(&halmutex);

#if CONFIG_UV_BOOTLOADER
	// if uv_bootloader is used, remap vector table to point to the new location
	SCB->VTOR = APP_START_ADDR;
#endif

	Chip_SYSCTL_PeriphReset(RESET_MUX);
	Chip_SYSCTL_PeriphReset(RESET_IOCON);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);
	Chip_SWM_Deinit();
	Chip_SWM_Init();
	Chip_GPIO_DeInit(LPC_GPIO);
	Chip_GPIO_Init(LPC_GPIO);

	// configure brown-out detection to reset the device
	LPC_SYSCON->BODCTRL = (2 << 0) | (1 << 4);


#if CONFIG_WDT
	_uv_wdt_init();
#endif

	// try to load non-volatile settings. If loading failed,
	// reset all peripherals which are depending on the
	// non-volatile settings.
	uv_errors_e e = uv_memory_load(MEMORY_COM_PARAMS);
	if (e) {
		_uv_rtos_hal_reset();
	}

#if CONFIG_CAN
	_uv_can_init();
#endif
#if CONFIG_TERMINAL_USBDVCOM
	vcom_init();
#endif


#if CONFIG_CANOPEN
	_uv_canopen_init(0);
#endif

#if CONFIG_ADC || CONFIG_ADC0 || CONFIG_ADC1
	_uv_adc_init();
#endif

#if CONFIG_UART0
	_uv_uart_init(UART0);
#endif
#if CONFIG_UART1
	_uv_uart_init(UART1);
#endif
#if CONFIG_UART2
	_uv_uart_init(UART2);
#endif
#if CONFI_UART3
	_uv_uart_init(UART3);
#endif

#if CONFIG_DAC
	_uv_dac_init();
#endif

#if CONFIG_SPI
	_uv_spi_init();
#endif

#if CONFIG_I2C
	_uv_i2c_init();
#endif

#if CONFIG_EMC
	_uv_emc_init();
#endif

#if CONFIG_LCD
	_uv_lcd_init();
#endif

#if CONFIG_PWM
	_uv_pwm_init();
#endif

#if CONFIG_EEPROM
	_uv_eeprom_init();
#endif

	uv_rtos_task_create(hal_task, "uv_hal",
			UV_RTOS_MIN_STACK_SIZE, NULL, CONFIG_HAL_TASK_PRIORITY, &hal_task_ptr);
	uv_rtos_task_create(canopen_task, "canopen",
			UV_RTOS_MIN_STACK_SIZE * 2, NULL, UV_RTOS_IDLE_PRIORITY + 1, NULL);

	rtos_init = true;
}



void uv_deinit(void) {
}


void uv_data_reset() {
#if CONFIG_CANOPEN
	_uv_canopen_reset();
#endif
}


void canopen_task(void *nullptr) {

	uint16_t step_ms = CONFIG_HAL_STEP_MS;

	uv_ts_st ts;
	uv_ts_init(&ts);

	while (true) {
#if CONFIG_CANOPEN
		uv_ts_step(&ts);

		_uv_canopen_step(uv_ts_get_step_ms(&ts));
#endif
		uv_rtos_task_delay(step_ms);
	}
}



void hal_task(void *nullptr) {

	uint16_t step_ms = CONFIG_HAL_STEP_MS;
	uv_ts_st ts;
	uv_ts_init(&ts);

	while (true) {
		uv_ts_step(&ts);

		_uv_rtos_halmutex_lock();

#if CONFIG_CAN
		_uv_can_hal_step(uv_ts_get_step_ms(&ts));
#endif

#if CONFIG_TERMINAL_CAN
		_uv_stdout_hal_step(uv_ts_get_step_ms(&ts));
#endif
#if CONFIG_UART
		_uv_uart_hal_step(uv_ts_get_step_ms(&ts));
#endif

		_uv_rtos_halmutex_unlock();

		uv_rtos_task_delay(step_ms);
	}
}





void _uv_rtos_hal_reset(void) {
	// reset all modules which depend on non-volatile settings
#if CONFIG_CANOPEN
		_uv_canopen_reset();
#endif
}
