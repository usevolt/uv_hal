/*
 * uw_rtos.c
 *
 *  Created on: Mar 18, 2016
 *      Author: usevolt
 */


#include "uw_rtos.h"

#include "uw_uart.h"
#include "uw_utilities.h"

typedef struct {
	void (*idle_task)(void *user_ptr);
	void (*tick_task)(void *user_ptr, unsigned int step_ms);
} this_st;

static volatile this_st _this = {
		.idle_task = NULL,
		.tick_task = NULL
};

#define this (&_this)

#if configUSE_IDLE_HOOK
uw_errors_e uw_rtos_add_idle_task(void (*task_function)(void *user_ptr)) {
	this->idle_task = task_function;
	return uw_err(ERR_NONE);
}
#endif

void uw_rtos_task_delay(unsigned int ms) {
	portTickType xDelayTime;

	xDelayTime = xTaskGetTickCount();
	vTaskDelayUntil(&xDelayTime, ms);
}


void vApplicationMallocFailedHook(void)
{
	__uw_log_error(uw_err(ERR_MALLOC_FAILURE | HAL_MODULE_RTOS));
	taskDISABLE_INTERRUPTS();
	for (;; ) {}
}

/* FreeRTOS application idle hook */
void vApplicationIdleHook(void)
{
	if (this->idle_task) {
		this->idle_task(__uw_get_user_ptr());
	}
}

/* FreeRTOS stack overflow hook */
void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName)
{
	(void) pxTask;
	(void) pcTaskName;

	__uw_log_error(uw_err(ERR_STACK_OVERFLOW | HAL_MODULE_RTOS));
	printf("task: %s\n\r", pcTaskName);
	/* Run time stack overflow checking is performed if
	   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	   function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();
	for (;; ) {}
}

/* FreeRTOS application tick hook */
void vApplicationTickHook(void) {
	if (this->tick_task) {
		this->tick_task(__uw_get_user_ptr(), portTICK_PERIOD_MS);
	}
}

