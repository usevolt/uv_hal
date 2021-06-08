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
#include <unistd.h>
#include <getopt.h>




void vAssertCalled( const char * const pcFileName,  unsigned long ulLine ) {
	printf("Assert in '%s', line %u\n", pcFileName, ulLine);
	exit(0);
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



int32_t uv_rtos_task_create(void (*task_function)(void *this_ptr), char *task_name,
		unsigned int stack_depth, void *this_ptr,
		unsigned int task_priority, uv_rtos_task_ptr* handle) {
	static unsigned int size = 0;
	size += stack_depth;
	if (size >= CONFIG_RTOS_HEAP_SIZE) {
		while(true) {
			printf("Out of memory\r");
		}
	}
	return xTaskCreate(task_function, (const char * const)task_name, stack_depth,
			this_ptr, task_priority, handle);
}



void uv_rtos_task_delay(unsigned int ms) {
	portTickType xDelayTime;

	xDelayTime = xTaskGetTickCount();
	vTaskDelayUntil(&xDelayTime, ms);
}


void vApplicationMallocFailedHook(void) {
	uv_disable_int();
	for (;; ) {}
}

/* FreeRTOS application idle hook */
void vApplicationIdleHook(void) {
	if (this->idle_task) {
		this->idle_task(__uv_get_user_ptr());
	}
	else {
		// wait for 1 tick step time to reduce the processor power consumption of the idle task
		usleep(1000000 / configTICK_RATE_HZ);
	}
}

/* FreeRTOS stack overflow hook */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char * pcTaskName) {
	(void) xTask;
	(void) pcTaskName;

	printf("stack overflow from task: %s\n", pcTaskName);
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




void uv_rtos_start_scheduler(void) {
	printf("The FreeRTOS scheduler started\n");
	vTaskStartScheduler();
	printf("The FreeRTOS scheduler stopped\n");
}


// Sets the default CAN dev
#define OPT_CAN		'c'
// displays the configuration UI that can be used to setup all file paths etc
#define OPT_UI		'w'
static struct option long_opts[] =
{
    {"can", required_argument, NULL, 'c'},
    {"ui", no_argument, NULL, 'w'},
    {NULL, 0, NULL, 0}
};


void uv_init_arg(void *device, int argc, char *argv[]) {

	char opts[128] = { };
	for (uint16_t i = 0; i < sizeof(long_opts) / sizeof(long_opts[0]); i++) {
		char str[4] = {};
		str[0] = long_opts[i].val;
		if (long_opts[i].has_arg == required_argument) {
			strcat(str, ":");
		}
		else if (long_opts[i].has_arg == optional_argument) {
			strcat(str, "::");
		}
		else {

		}
		strcat(opts, str);
	}
	int ch;
	// loop over all of the options
	while ((ch = getopt_long(argc, argv, opts, long_opts, NULL)) != -1)
	{
	    // check to see if a single character or long option came through
	    switch (ch)
	    {
	         case OPT_CAN:
	        	 printf("Can dev set '%s'\n", optarg);
	        	 uv_can_set_dev(optarg);
	             break;
#if CONFIG_UI
	         case OPT_UI:
	        	 printf("Showing the configuration UI\n");
	        	 ui_x11_confwindow_exec();
	        	 break;
#endif
	         case '?':
	        	 exit(0);
	             break;
	         default:
	        	 break;
	    }
	}

	uv_init(device);
}


void uv_init(void *device) {
	uv_set_application_ptr(device);
	uv_mutex_init(&halmutex);

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

	uv_rtos_task_create(hal_task, "uv_hal", UV_RTOS_MIN_STACK_SIZE, NULL, 0xFFFF, NULL);
}



void uv_deinit(void) {
}


void uv_data_reset() {
#if CONFIG_CANOPEN
	_uv_canopen_reset();
#endif
}




void hal_task(void *nullptr) {

	uint16_t step_ms = CONFIG_HAL_STEP_MS;
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



void _uv_rtos_hal_reset(void) {
	// reset all modules which depend on non-volatile settings
#if CONFIG_CANOPEN
		_uv_canopen_reset();
#endif
}
