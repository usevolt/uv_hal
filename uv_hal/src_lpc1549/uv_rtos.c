/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
#if CONFIG_WDT
#include "uv_wdt.h"
#endif
#if CONFIG_ADC || CONFIG_ADC0 || CONFIG_ADC1
#include "uv_adc.h"
#endif
#include "uv_dac.h"

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




#define this (&_this)


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
void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName)
{
	(void) pxTask;
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





void uv_init(void *device) {
	uv_set_application_ptr(device);
	uv_mutex_init(&halmutex);

#if CONFIG_TARGET_LPC1549
	Chip_SYSCTL_PeriphReset(RESET_MUX);
	Chip_SYSCTL_PeriphReset(RESET_IOCON);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);
	Chip_SWM_Init();
	Chip_GPIO_Init(LPC_GPIO);
#endif

#if CONFIG_WDT
	_uv_wdt_init();
#endif

	// try to load non-volatile settings. If loading failed,
	// reset all peripherals which are depending on the
	// non-volatile settings.
	if (_uv_memory_hal_load()) {
#if CONFIG_CANOPEN
		_uv_canopen_reset();
#endif
	}

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


#if CONFIG_CANOPEN
	_uv_canopen_init();
#endif

#if CONFIG_ADC || CONFIG_ADC0 || CONFIG_ADC1
	_uv_adc_init();
#endif

#if CONFIG_DAC
	_uv_dac_init();
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


#if CONFIG_TARGET_LPC11C14
	// delay of half a second on start up.
	// Makes entering ISP mode possible on startup before freeRTOS scheduler is started
	_delay_ms(500);
	char c;

	uv_errors_e e;
	while (true) {
		if ((e = uv_uart_get_char(UART0, &c))) {
			break;
		}
		if (c == '?') {
			uv_enter_ISP_mode();
		}
	}
#endif


	uv_rtos_task_create(hal_task, "uv_hal", UV_RTOS_MIN_STACK_SIZE, NULL, CONFIG_HAL_TASK_PRIORITY, NULL);
}



void uv_deinit(void) {
#if CONFIG_TARGET_LINUX
	uv_can_deinit();
#endif
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



