/*
 * CONFIG_RTOS.h
 *
 *  Created on: Mar 17, 2016
 *      Author: usevolt
 */

#ifndef CONFIG_RTOS_H_
#define CONFIG_RTOS_H_


#include "uw_hal_config.h"

#if CONFIG_RTOS

/// @file: A wrapper for FreeRTOS real time operating system.
/// Only usable in builds which defined CONFIG_RTOS preprocessor symbol
///
/// Since FreeRTOS uses many build time configurations in it,
/// the application program should include FreeRTOS files in
/// source and include paths, as well as provide a FreeRTOSConfig.h
/// header file for configuring the RTOS.
///
///

#include "uw_errors.h"
#include "FreeRTOS.h"
#include "task.h"

#define uw_rtos_min_stack_size 			configMINIMAL_STACK_SIZE
#define uw_rtos_idle_priority			tskIDLE_PRIORITY


#if CONFIG_RTOS
/// @brief: Disables all interrupt. This shouldn't be called from
/// interrupt routines, use uw_disable_int_ISR instead!
#define uw_disable_int()		taskENTER_CRITICAL()
/// @brief: Disabled all interrupts. This is meant to be called from
/// inside interrupt routines.
#define uw_disable_int_ISR()	taskENTER_CRITICAL_FROM_ISR()
/// @brief: Enables all interrupts. This shouldn't be called from
/// interrupt routines, use uw_enable_int_ISR instead!
#define uw_enable_int()			taskEXIT_CRITICAL()
/// @brief: Enabled all interrupts. This is meant to be called from
/// inside interrupt routines.
#define uw_enable_int_ISR()		taskEXIT_CRITICAL_FROM_ISR(1)
#endif


typedef xTaskHandle uw_rtos_task_handle;

/// @brief: e.g. FreeRTOS applicationIdleHook, this function can be used
/// to add a idle task function. Idle function will be called every time
/// the application is performing the idle task loop.
///
/// @param task_function: The task function pointer. Takes user_ptr as a parameter.
/// Refer to uw_utilities.h for more details.
uw_errors_e uw_rtos_add_idle_task(void (*task_function)(void *user_ptr));



/// @brief: Wrapper for FreeRTOS vTaskDelaiUntil-function. Use this to
/// set task to wait for 'ms' milliseconds before continuing execution.
void uw_rtos_task_delay(unsigned int ms);


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
static inline void uw_rtos_task_create(void (*task_function)(void *this_ptr), char *task_name,
		unsigned int stack_depth, void *this_ptr,
		unsigned int task_priority, uw_rtos_task_handle* handle) {
	xTaskCreate(task_function, (const char * const)task_name, stack_depth,
			this_ptr, task_priority, handle);
}


/// @brief: Remove a task from the RTOS kernels management. The task being
/// deleted will be removed from all ready, blocked, suspended and event lists
static inline void uw_rtos_task_delete(uw_rtos_task_handle task) {
	vTaskDelete(task);
}



/// @brief: Starts the RTOS scheduler. After calling this, the
/// RTOS kernel has control over which tasks are executed and when.
/// In normal operation this function never returns.
static inline void uw_rtos_start_scheduler(void) {
	vTaskStartScheduler();
}

/// @brief: Stops the RTOS scheduler. After calling this,
/// the RTOS stops functioning.
static inline void uw_rtos_end_scheduler(void) {
	vTaskEndScheduler();
}


#endif


#endif /* CONFIG_RTOS_H_ */

