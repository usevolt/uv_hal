/*
 * uv_rtos.c
 *
 *  Created on: Mar 18, 2016
 *      Author: usevolt
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
#if CONFIG_WDT
#include "uv_wdt.h"
#endif
#if CONFIG_ADC
#include "uv_adc.h"
#endif

typedef struct {
	void (*idle_task)(void *user_ptr);
	void (*tick_task)(void *user_ptr, unsigned int step_ms);
} this_st;

static volatile this_st _this = {
		.idle_task = NULL,
		.tick_task = NULL
};


void uv_rtos_mutex_unlock(uv_mutex *this) {
	__disable_irq();
	this->locked = false;
	__enable_irq();
}


uint32_t uv_rtos_mutex_lock(uv_mutex *this) {
	uint32_t time = 0;
	while (true) {
		__disable_irq();
		if (this->locked) {
			time++;
		}
		else {
			this->locked = true;
			__enable_irq();
			return time;
		}
		__enable_irq();

		uv_rtos_task_delay(1);
	}
}



#define this (&_this)


/// @brief: Task function which takes care of calling several hal librarys module
/// hal step functions
void hal_task(void *);


void uv_rtos_task_create(void (*task_function)(void *this_ptr), char *task_name,
		unsigned int stack_depth, void *this_ptr,
		unsigned int task_priority, uv_rtos_task_ptr* handle) {
	static unsigned int size = 0;
	size += stack_depth;
	if (size >= CONFIG_RTOS_HEAP_SIZE) {
		while(true) {
			__NOP();
		}
	}
	xTaskCreate(task_function, (const char * const)task_name, stack_depth,
			this_ptr, task_priority, handle);
}



#if configUSE_IDLE_HOOK
uv_errors_e uv_rtos_add_idle_task(void (*task_function)(void *user_ptr)) {
	this->idle_task = task_function;
	return uv_err(ERR_NONE);
}
#endif

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


bool rtos_init = false;



void uv_init(void *device) {
	uv_set_application_ptr(device);

#if CONFIG_TARGET_LPC1549
	Chip_SYSCTL_PeriphReset(RESET_MUX);
	Chip_SWM_Init();
	Chip_GPIO_Init(LPC_GPIO);
#endif

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
	if (_uv_memory_hal_load()) {
#if CONFIG_CANOPEN
		_uv_canopen_reset();
#endif
	}

#if CONFIG_CANOPEN
	_uv_canopen_init();
#endif

#if CONFIG_ADC
	_uv_adc_init();
#endif

#if CONFIG_SPI
	_uv_spi_init();
#endif

#if CONFIG_DMA
	_uv_dma_init();
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

	uv_rtos_task_create(hal_task, "uv_hal", UV_RTOS_MIN_STACK_SIZE, NULL, 0xFFFF, NULL);
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
#if CONFIG_CAN
	_uv_can_hal_step(step_ms);
#endif

#if CONFIG_TERMINAL_CAN
	_uv_stdout_hal_step(step_ms);
#endif

#if CONFIG_CANOPEN
	_uv_canopen_step(step_ms);
#endif
	uv_rtos_task_delay(step_ms);
	}

}



