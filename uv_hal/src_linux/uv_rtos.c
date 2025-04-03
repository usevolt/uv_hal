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
#include <signal.h>
#include "main.h"


#if !defined(PRINT)
#define PRINT(...) printf(__VA_ARGS__)
#endif


void vAssertCalled( const char * const pcFileName,  unsigned long ulLine ) {
	PRINT("Assert in '%s', line %u\n", pcFileName, ulLine);
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
/// @brief: C signal callback
void signal_callb(int signum);



int32_t uv_rtos_task_create(void (*task_function)(void *this_ptr), char *task_name,
		unsigned int stack_depth, void *this_ptr,
		unsigned int task_priority, uv_rtos_task_ptr* handle) {
	static unsigned int size = 0;
	size += stack_depth;
	if (size >= CONFIG_RTOS_HEAP_SIZE) {
		while(true) {
			PRINT("Task creation failed: Out of memory\r");
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

	PRINT("stack overflow from task: %s\n", pcTaskName);
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
	PRINT("The FreeRTOS scheduler started\n");
	vTaskStartScheduler();
	PRINT("The FreeRTOS scheduler stopped\n");
}


// Sets the default CAN dev
#define OPT_CAN		'c'
// displays the configuration UI that can be used to setup all file paths etc
#define OPT_UI		'u'
// sets the non-volatile memory file path. Default is set in uv_memory.c
#define OPT_NONVOL	'v'
// sets the eeprom memory file path. Default is set in uv_eeprom.c
#define OPT_EEPROM	'e'
// sets the CANopen NODEID of this device. Force overwrites the nodeid
// set in non-volatile parameters
#define OPT_NODEID	'n'
static int8_t arg_nodeid = 0;
static struct option long_opts[] =
{
    {"can", required_argument, NULL, OPT_CAN},
    {"ui", no_argument, NULL, OPT_UI},
	{"nonvol", required_argument, NULL, OPT_NONVOL},
	{"eeprom", required_argument, NULL, OPT_EEPROM},
	{"nodeid", required_argument, NULL, OPT_NODEID},
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
#if CONFIG_CAN
	         case OPT_CAN:
	        	 PRINT("Can dev set '%s'\n", optarg);
	        	 uv_can_set_dev(optarg);
	             break;
#endif
#if CONFIG_UI
	         case OPT_UI:
	        	 PRINT("Showing the configuration UI\n");
	        	 uv_ui_confwindow_exec();
	        	 break;
#endif
	         case OPT_NONVOL:
	        	 PRINT("Setting the non-volatile memory file path to '%s'\n", optarg);
	        	 uv_memory_set_nonvol_filepath(optarg);
	        	 break;
#if CONFIG_EEPROM
	         case OPT_EEPROM:
	        	 PRINT("Setting the non-volatile memory file path to '%s'\n", optarg);
	        	 uv_eeprom_set_filepath(optarg);
	        	 break;
#endif
	         case OPT_NODEID: {
				 arg_nodeid = strtol(optarg, NULL, 0);
	        	 PRINT("Setting nodeid to 0x%x\n", arg_nodeid);
				 break;
	         }
	         case '?':
	             break;
	         default:
				 PRINT("Defined but not used argument '%c'\n", ch);
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
	uint8_t last_nodeid = dev.data_start.id;
	_uv_canopen_init(arg_nodeid);
	if (arg_nodeid) {
		// update all PDO's which where mapped for the previous node id
		uv_enter_critical();
		for (uint8_t i = 0; i < CONFIG_CANOPEN_TXPDO_COUNT; i++) {
			canopen_txpdo_com_parameter_st *com =
					&dev.data_start.canopen_data.txpdo_coms[i];
			if ((com->cob_id & 0x7F) == last_nodeid) {
				com->cob_id &= ~(0x7F);
				com->cob_id |= arg_nodeid;
			}
		}
		for (uint8_t i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
			canopen_rxpdo_com_parameter_st *com =
					&dev.data_start.canopen_data.rxpdo_coms[i];
			if ((com->cob_id & 0x7F) == last_nodeid) {
				com->cob_id &= ~(0x7F);
				com->cob_id |= arg_nodeid;
			}
		}
		uv_can_clear_rx_messages(CONFIG_CANOPEN_CHANNEL);
		uv_canopen_config_rx_msgs();
		uv_exit_critical();
	}
#endif

	uv_rtos_task_create(hal_task, "uv_hal", UV_RTOS_MIN_STACK_SIZE, NULL, 0xFFFF, NULL);

	// Register signal and signal handler
	signal(SIGINT, signal_callb);
}



void signal_callb(int signum) {
	PRINT("Caught signal %u\n", signum);

	uv_deinit();
   // Terminate program
   exit(signum);
}

void uv_deinit(void) {
#if CONFIG_UI
	uv_ui_destroy();
#endif
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
