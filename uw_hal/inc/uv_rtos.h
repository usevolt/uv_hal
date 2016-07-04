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

#include "uv_errors.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


#if !defined(CONFIG_RTOS_HEAP_SIZE)
#error "CONFIG_RTOS_HEAP_SIZE not defined. It should define the number of bytes reserved to be used\
 as a RTOS task memory."
#endif


#define UV_RTOS_MIN_STACK_SIZE 			configMINIMAL_STACK_SIZE
#define UV_RTOS_IDLE_PRIORITY			tskIDLE_PRIORITY

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
typedef xSemaphoreHandle	uv_rtos_smphr_ptr;
typedef xQueueHandle		uv_rtos_queue_ptr;







#if configUSE_IDLE_HOOK
/// @brief: e.g. FreeRTOS applicationIdleHook, this function can be used
/// to add a idle task function. Idle function will be called every time
/// the application is performing the idle task loop.
///
/// @param task_function: The task function pointer. Takes user_ptr as a parameter.
/// Refer to uv_utilities.h for more details.
uv_errors_e uv_rtos_add_idle_task(void (*task_function)(void *user_ptr));
#endif







/// @bief: Creates and returns a binary semaphore.
/// @note: Binary sempahores are good for passing information between tasks and
/// ISR's. Note that ISR's can only give and tasks can
/// only take binary semaphores.
static inline uv_rtos_smphr_ptr uv_rtos_smphr_create_binary(void) {
	return xSemaphoreCreateBinary();
}

/// @brief: "Gives" or "releases" the semaphore. This makes possible for
/// other tasks to take the ownership of the semaphore.
/// @note: This function shouldn't be called from ISR's!
/// Use uv_rtos_smprh_give_ISR instead.
static inline void uv_rtos_smphr_give(uv_rtos_smphr_ptr handle) {
	xSemaphoreGive(handle);
}

/// @brief: "Gives" or "releases" the semaphore. This makes possible for
/// other tasks to take the ownership of the semaphore.
/// @note: This function should be called only from ISR's!
static inline void uv_rtos_smphr_give_ISR(uv_rtos_smphr_ptr handle) {
	xSemaphoreGiveFromISR(handle, NULL);
}

/// @brief: Attempts to take the ownership of the semaphore. The function
/// waits 'max_wait_tick_count' ticks and returns true if the semaphore could be taken,
/// false otherwise.
/// @note: This function shouldn't be called from ISR's!
/// Use uv_rtos_smprh_give_ISR instead.
static inline bool uv_rtos_smphr_take(uv_rtos_smphr_ptr handle, unsigned int max_wait_tick_count) {
	return xSemaphoreTake(handle, max_wait_tick_count);
}

/// @brief:A version of xSemaphoreTake() that can be called from an ISR.
/// Unlike xSemaphoreTake(), xSemaphoreTakeFromISR() does not permit a
/// block time to be specified.
/// @note: This function should be called only from ISR's!
static inline bool uv_rtos_smphr_take_ISR(uv_rtos_smphr_ptr handle) {
	return xSemaphoreTakeFromISR(handle, NULL);
}









/// @brief: Creates and returns a queue. Queues are a way of sending information
/// between tasks.
///
/// @return: Queue created, NULL if failed.
///
/// @param queue_length: The length of how many items the queue can contain
/// @param type_length: The byte length of a single item
static inline uv_rtos_queue_ptr uv_rtos_queue_create(unsigned int queue_length, uint8_t type_length) {
	return xQueueCreate(queue_length, type_length);
}


/// @brief: Resets (clears) the queue to its original state
static inline void uv_rtos_queue_clear(uv_rtos_queue_ptr queue) {
	xQueueReset(queue);
}


/// @brief: Returns true if the queue is full
/// @note: This function shouldn't be called from ISR's!
/// Use uv_rtos_queue_is_full_ISR instead!
static inline bool uv_rtos_queue_is_full(uv_rtos_queue_ptr queue) {
	return !(uxQueueSpacesAvailable(queue));
}


/// @brief: Returns true if the queue is full
/// @note: This function should be called only from ISR's!
static inline bool uv_rtos_queue_is_full_ISR(uv_rtos_queue_ptr queue) {
	return xQueueIsQueueFullFromISR(queue);
}


/// @brief: Pushes new data to back of the queue.
///
/// @note: This function shouldn't be called from ISR's!
///
/// @param queue: The handle to the queue on which the item is to be posted
/// @param data: A pointer to the item that is to be placed on the queue.
/// @param ticks_to_wait:  maximum amount of time the task should block waiting
/// for space to become available on the queue, should it already be full
static inline bool uv_rtos_queue_push(uv_rtos_queue_ptr queue, const void * data,
		unsigned int ticks_to_wait) {
	return xQueueSend(queue, data, ticks_to_wait);
}


/// @brief: Pushes new data to back of the queue.
///
/// @note: This function should be called only from ISR's!
///
/// @param queue: The handle to the queue on which the item is to be posted
/// @param data: A pointer to the item that is to be placed on the queue.
/// @param ticks_to_wait:  maximum amount of time the task should block waiting
/// for space to become available on the queue, should it already be full
static inline bool uv_rtos_queue_push_ISR(uv_rtos_queue_ptr queue, const void * data) {
	return xQueueSendToBackFromISR(queue, data, NULL);
}




/// @brief: Receives the data from queue. The data is received in FIFO order.
///
/// @note: This function shouldn't be called from ISR's!
///
/// @param queue: The handle to the queue from which the item is to be received
/// @param data: Pointer to the buffer into which the received item will be copied
/// @param ticks_to_wait: The maximum amount of time the task should block waiting for an item
/// to receive should the queue be empty at the time of the call
static inline bool uv_rtos_queue_pop(uv_rtos_queue_ptr queue, void *data, unsigned int ticks_to_wait) {
	return xQueueReceive(queue, data, ticks_to_wait);
}

/// @brief: Receives the data from queue. The data is received in FIFO order.
///
/// @note: This function should be called only from ISR's!
///
/// @param queue: The handle to the queue from which the item is to be received
/// @param data: Pointer to the buffer into which the received item will be copied
/// @param ticks_to_wait: The maximum amount of time the task should block waiting for an item
/// to receive should the queue be empty at the time of the call
static inline bool uv_rtos_queue_pop_ISR(uv_rtos_queue_ptr queue, void *data) {
	return xQueueCRReceiveFromISR(queue, data, NULL);
}










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
static inline void uv_rtos_task_create(void (*task_function)(void *this_ptr), char *task_name,
		unsigned int stack_depth, void *this_ptr,
		unsigned int task_priority, uv_rtos_task_ptr* handle) {
	xTaskCreate(task_function, (const char * const)task_name, stack_depth,
			this_ptr, task_priority, handle);
}


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







#endif


#endif /* CONFIG_RTOS_H_ */

