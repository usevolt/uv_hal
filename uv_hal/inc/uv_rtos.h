/*
 * CONFIG_RTOS.h
 *
 *  Created on: Mar 17, 2016
 *      Author: usevolt
 */

#ifndef CONFIG_RTOS_H_
#define CONFIG_RTOS_H_


#include "uv_hal_config.h"
#include <stdbool.h>


/// @file: A wrapper for FreeRTOS real time operating system.
/// Only usable in builds which defined CONFIG_RTOS preprocessor symbol
///
/// Since FreeRTOS uses many build time configurations in it,
/// the application program should include FreeRTOS files in
/// source and include paths, as well as provide a FreeRTOSConfig.h
/// header file for configuring the RTOS.
///
///

#include "uv_errors.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#if !defined(CONFIG_RTOS_HEAP_SIZE)
#error "CONFIG_RTOS_HEAP_SIZE not defined. It should define the number of bytes reserved to be used\
 as a RTOS task memory."
#endif


#define UV_RTOS_MIN_STACK_SIZE 			configMINIMAL_STACK_SIZE
#define UV_RTOS_IDLE_PRIORITY			(tskIDLE_PRIORITY + 1)

/// @brief: The tick timer period time in ms
#define UV_RTOS_TICK_PERIOD_MS			portTICK_PERIOD_MS
/// @brief: Maximum delay time in ticks
#define UV_RTOS_MAX_DELAY				portMAX_DELAY


#if CONFIG_RTOS
/// @brief: Disables all interrupt. This shouldn't be called from
/// interrupt routines, use uv_disable_int_ISR instead!
#define uv_disable_int()		taskENTER_CRITICAL()
/// @brief: Disabled all interrupts. This is meant to be called from
/// inside interrupt routines.
#define uv_disable_int_ISR()	taskENTER_CRITICAL_FROM_ISR()
/// @brief: Enables all interrupts. This shouldn't be called from
/// interrupt routines, use uv_enable_int_ISR instead!
#define uv_enable_int()			taskEXIT_CRITICAL()
/// @brief: Enabled all interrupts. This is meant to be called from
/// inside interrupt routines.
#define uv_enable_int_ISR()		taskEXIT_CRITICAL_FROM_ISR(1)
#endif


typedef xTaskHandle 		uv_rtos_task_ptr;





/// @brief: Initializes the real time OS. Basicly sets up a HAL task which takes care
/// of several hal module step functions
///
/// @param device: Pointer to the main application data structure. This is the struct which will
/// be given as the **me** parameter to the HAL library's callback functions.
void uv_init(void *device);


/// @brief: Resets the non-volatile data from all HAL modules to defaults
void uv_data_reset();

extern bool rtos_init;
/// @brief: Returns true when the rtos HAL task is succesfully running and all peripherals should be
/// initialized.
static inline bool uv_rtos_initialized() {
	return rtos_init;
}


#if configUSE_IDLE_HOOK
/// @brief: e.g. FreeRTOS applicationIdleHook, this function can be used
/// to add a idle task function. Idle function will be called every time
/// the application is performing the idle task loop.
///
/// @param task_function: The task function pointer. Takes user_ptr as a parameter.
/// Refer to uv_utilities.h for more details.
uv_errors_e uv_rtos_add_idle_task(void (*task_function)(void *user_ptr));
#endif





typedef struct {
	bool locked;
} uv_mutex;


static inline void uv_rtos_mutex_init(uv_mutex* this) {
	this->locked = false;
}

/// @brief: Unlocks the mutex
void uv_rtos_mutex_unlock(uv_mutex *this);

/// @brief: Waits until the mutex has been locked. Returns time in milliseconds
/// how long it took to lock the mutex
uint32_t uv_rtos_mutex_lock(uv_mutex *this);




/// @brief: Wrapper for FreeRTOS vTaskDelaiUntil-function. Use this to
/// set task to wait for 'ms' milliseconds before continuing execution.
void uv_rtos_task_delay(unsigned int ms);


/// @brief: Forces a context switch
static inline void uv_rtos_task_yield(void) {
	taskYIELD();
}








/// @brief: Create a new task and add it to the list of tasks that are ready to run.
///
/// @param task_function: The function which will be executed as a task
/// @param task_name: A name for the task. The maximum length for the name is
/// defined in the FreeRTOSConfig.h file.
/// @param stack_depth: The maximum size for the task's stack, defined in words
/// @param this_ptr: A user definable pointer which will be passed to the task function.
/// @param task_priority: The priority of the task. Bigger value means higher priority.
/// @param handle: A optional return handle for the created task. Will be set to NULL
/// if task creation failed.
void uv_rtos_task_create(void (*task_function)(void *this_ptr), char *task_name,
		unsigned int stack_depth, void *this_ptr,
		unsigned int task_priority, uv_rtos_task_ptr* handle);


/// @brief: Remove a task from the RTOS kernels management. The task being
/// deleted will be removed from all ready, blocked, suspended and event lists
static inline void uv_rtos_task_delete(uv_rtos_task_ptr task) {
	vTaskDelete(task);
}







/// @brief: Starts the RTOS scheduler. After calling this, the
/// RTOS kernel has control over which tasks are executed and when.
/// In normal operation this function never returns.
static inline void uv_rtos_start_scheduler(void) {
	vTaskStartScheduler();
}

/// @brief: Stops the RTOS scheduler. After calling this,
/// the RTOS stops functioning.
static inline void uv_rtos_end_scheduler(void) {
	vTaskEndScheduler();
}









#endif /* CONFIG_RTOS_H_ */

