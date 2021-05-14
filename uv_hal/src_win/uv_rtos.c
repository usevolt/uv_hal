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
#include "uv_pwm.h"
#include "uv_eeprom.h"
#include "uv_emc.h"
#include "uv_rtc.h"
#include "uv_ft81x.h"
#include "unistd.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#if CONFIG_WDT
#include "uv_wdt.h"
#endif
#if CONFIG_ADC || CONFIG_ADC0 || CONFIG_ADC1
#include "uv_adc.h"
#endif

typedef struct {
	pthread_t thread;
	void (*function_ptr)(void*);
	void *param;
	char name[64];
} thread_st;



// wrapper function for pthreads
static void *thread_func(void *thread_ptr) {
	thread_st *thread = thread_ptr;

	thread->function_ptr(thread->param);

	return NULL;
}

typedef struct {
	void (*idle_task)(void *user_ptr);
	void (*tick_task)(void *user_ptr, unsigned int step_ms);

} this_st;
bool rtos_init = false;
static bool initialized = false;
static bool scheduler_running = false;

static volatile this_st _this = {
		.idle_task = NULL,
		.tick_task = NULL
};



uv_mutex_st halmutex;






#define this ((this_st*) &_this)


uv_errors_e uv_rtos_add_idle_task(void (*task_function)(void *user_ptr)) {
	this->idle_task = task_function;

	return ERR_NONE;
}

bool uv_rtos_idle_task_set(void) {
	return (this->idle_task == NULL) ? false : true;
}



/// @brief: Task function which takes care of calling several hal librarys module
/// hal step functions
void hal_task(void *);




void uv_rtos_start_scheduler(void) {
	printf("The FreeRTOS scheduler started\n");
	vTaskStartScheduler();
	printf("The FreeRTOS scheduler stopped\n");
}





#if configUSE_IDLE_HOOK
uv_errors_e uv_rtos_add_idle_task(void (*task_function)(void *user_ptr)) {
	this->idle_task = task_function;
	return uv_err(ERR_NONE);
}
#endif

void uv_rtos_task_delay(unsigned int ms) {
	struct timespec t;
	t.tv_sec = ms / 1000;
	t.tv_nsec = (ms * 1000000) % 1000000000;
	nanosleep(&t, NULL);
}


void vApplicationMallocFailedHook(void)
{
	uv_disable_int();
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
//void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName)
//{
//	(void) pxTask;
//	(void) pcTaskName;
//
//	printf("stack overflow from task: %s\n", pcTaskName);
//	/* Run time stack overflow checking is performed if
//	   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
//	   function is called if a stack overflow is detected. */
//	taskDISABLE_INTERRUPTS();
//	for (;; ) {}
//}

/* FreeRTOS application tick hook */
//void vApplicationTickHook(void) {
//	if (this->tick_task) {
//		this->tick_task(__uv_get_user_ptr(), portTICK_PERIOD_MS);
//	}
//}







void uv_init(void *device) {
	uv_set_application_ptr(device);
	uv_mutex_init(&halmutex);


#if CONFIG_WDT
	_uv_wdt_init();
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

#if CONFIG_CAN
	_uv_can_init();
#endif

	// try to load non-volatile settings. If loading failed,
	// reset all peripherals which are denpending on the
	// non-volatile settings.
	if (uv_memory_load(MEMORY_COM_PARAMS)) {
#if CONFIG_CANOPEN
		_uv_canopen_reset();
#endif
	}

#if CONFIG_CANOPEN
	_uv_canopen_init();
#endif

#if CONFIG_ADC || CONFIG_ADC0 || CONFIG_ADC1
	_uv_adc_init();
#endif

#if CONFIG_SPI
	_uv_spi_init();
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

#if CONFIG_RTC
	_uv_rtc_init();
#endif



	uv_rtos_task_create(hal_task, "uv_hal", UV_RTOS_MIN_STACK_SIZE, NULL, 0xFFFF, NULL);
}



void uv_deinit(void) {
	uv_can_close();
}


void uv_data_reset() {
#if CONFIG_CANOPEN
	_uv_canopen_reset();
#endif
}




void hal_task(void *nullptr) {

	uint16_t step_ms = 2;
	rtos_init = true;

	while (true) {

		_uv_rtos_halmutex_lock();

#if CONFIG_CAN
		_uv_can_hal_step(step_ms);
#endif

#if CONFIG_TERMINAL_CAN
		_uv_stdout_hal_step(step_ms);
#endif

#if CONFIG_CANOPEN
		_uv_canopen_step(step_ms);
#endif

		_uv_rtos_halmutex_unlock();

		uv_rtos_task_delay(step_ms);
	}
}



