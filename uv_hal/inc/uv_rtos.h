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

#ifndef CONFIG_RTOS_H_
#define CONFIG_RTOS_H_


#include "uv_hal_config.h"
#include <stdbool.h>
#include <stdint.h>


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
#if !CONFIG_TARGET_LINUX && !CONFIG_TARGET_WIN
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#else
#include "sched.h"
#include "pthread.h"
#endif

#if !defined(CONFIG_RTOS_HEAP_SIZE)
#error "CONFIG_RTOS_HEAP_SIZE not defined. It should define the number of bytes reserved to be used\
 as a RTOS task memory."
#endif
#if !defined(CONFIG_UV_BOOTLOADER)
#error "CONFIG_UV_BOOTLOADER shoud be defines as 1 if Usevolt bootloader is used for this device.\
 In this case the bootloader takes care of configuring the system oscillator (external crystal).\
 Otherwise this should be defined as 0."
#endif
#if !defined(CONFIG_HAL_TASK_PRIORITY)
#define CONFIG_HAL_TASK_PRIORITY	0xFFFD
#endif

#if !defined(CONFIG_HAL_STEP_MS)
#define CONFIG_HAL_STEP_MS			2
#endif



#define UV_RTOS_MIN_STACK_SIZE 			configMINIMAL_STACK_SIZE
#define UV_RTOS_IDLE_PRIORITY			(tskIDLE_PRIORITY + 1)

/// @brief: The tick timer period time in ms
#define UV_RTOS_TICK_PERIOD_MS			portTICK_PERIOD_MS
/// @brief: Maximum delay time in ticks
#define UV_RTOS_MAX_DELAY				portMAX_DELAY


/// @brief: Disables all interrupt. This shouldn't be called from
/// interrupt routines, use uv_disable_int_ISR instead!
#if (CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1549 || CONFIG_TARGET_LPC1785)
#define uv_disable_int()		__disable_irq()
#elif CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
#define uv_disable_int()
#endif
/// @brief: Disabled all interrupts. This is meant to be called from
/// inside interrupt routines.
#define uv_disable_int_ISR()	taskENTER_CRITICAL_FROM_ISR()
/// @brief: Enables all interrupts. This shouldn't be called from
/// interrupt routines, use uv_enable_int_ISR instead!
#if (CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1549 || CONFIG_TARGET_LPC1785)
#define uv_enable_int()			__enable_irq()
#elif CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
#define uv_enable_int()
#endif
/// @brief: Enabled all interrupts. This is meant to be called from
/// inside interrupt routines.
#define uv_enable_int_ISR()		taskEXIT_CRITICAL_FROM_ISR(1)

#define uv_enter_critical()		taskENTER_CRITICAL()

#define uv_exit_critical()		taskEXIT_CRITICAL()



#if !CONFIG_TARGET_LINUX && !CONFIG_TARGET_WIN
typedef xTaskHandle 		uv_rtos_task_ptr;


/// @brief: Wrapper for FreeRTOS mutex semaphore
typedef SemaphoreHandle_t 	uv_mutex_st;

/// @brief: Initializes a mutex. Must be called before uv_mutex_lock and uv_mutex_unlock
static inline void uv_mutex_init(uv_mutex_st *mutex) {
	*mutex = xSemaphoreCreateBinary();
	xSemaphoreGive(*mutex);
}

/// @brief: Locks the mutex. No one else can lock the mutex before it is unlocked.
static inline void uv_mutex_lock(uv_mutex_st *mutex) {
	xSemaphoreTake(*mutex, portMAX_DELAY);
}

/// @brief: Tries to lock the mutex for *wait_ms* milliseconds
///
/// @return: True if the mutex could be locked, false otherwise
static inline bool uv_mutex_lock_ms(uv_mutex_st *mutex, int32_t wait_ms) {
	return xSemaphoreTake(*mutex, wait_ms / portTICK_PERIOD_MS);
}

/// @brief: Unlocks the mutex after which others can lock it again.
static inline void uv_mutex_unlock(uv_mutex_st *mutex) {
	xSemaphoreGive(*mutex);
}

static inline bool uv_mutex_lock_isr(uv_mutex_st *mutex) {
	return xSemaphoreTakeFromISR(*mutex, NULL);
}

void uv_mutex_unlock_isr(uv_mutex_st *mutex);

typedef QueueHandle_t uv_queue_st;

/// @brief: Creates a FreeRTOS thread-safe queue
///
/// @param queue_len: The length of the queue in element count
/// @param element_size: the size of an individual element in bytes
static inline void uv_queue_init(uv_queue_st *this,
		int32_t queue_len, uint32_t element_size) {
	*this = xQueueCreate(queue_len, element_size);
}


/// @brief: Returns the count of elements currently in the queue
static inline int32_t uv_queue_get_len(uv_queue_st *this) {
	return uxQueueMessagesWaiting(*this);
}

/// @brief: Peeks into a buffer without removing the element
///
/// @param dest: Destination address where to copy the element
/// @param wait_ms: Wait time in milliseconds
uv_errors_e uv_queue_peek(uv_queue_st *this, void *dest, int32_t wait_ms);


/// @brief: Pushes into the queue. If the queue was full, waits for *wait_ms*
/// milliseconds for it to get empty.
uv_errors_e uv_queue_push(uv_queue_st *this, void *src, int32_t wait_ms);

/// @brief: Pushes into a queue. To be used from ISR routines!
uv_errors_e uv_queue_push_isr(uv_queue_st *this, void *src);


/// @brief: Removes an element from the queue
uv_errors_e uv_queue_pop(uv_queue_st *this, void *dest, int32_t wait_ms);

/// @brief: Removes from a queue. To be used from ISR routines!
uv_errors_e uv_queue_pop_isr(uv_queue_st *this, void *dest);


#else

#define configMINIMAL_STACK_SIZE		10
#define tskIDLE_PRIORITY				1

typedef void			uv_rtos_task_ptr;

typedef pthread_mutex_t uv_mutex_st;

/// @brief: Initializes a mutex. Must be called before uv_mutex_lock and uv_mutex_unlock
static inline void uv_mutex_init(uv_mutex_st *mutex) {
	pthread_mutex_init(mutex, NULL);
}


/// @brief: Locks the mutex. No one else can lock the mutex before it is unlocked.
static inline void uv_mutex_lock(uv_mutex_st *mutex) {
	pthread_mutex_lock(mutex);
}

/// @brief: Unlocks the mutex after which others can lock it again.
static inline void uv_mutex_unlock(uv_mutex_st *mutex) {
	pthread_mutex_unlock(mutex);
}

#endif




/// @brief: Initializes the real time OS. Basicly sets up a HAL task which takes care
/// of several hal module step functions
///
/// @param device: Pointer to the main application data structure. This is the struct which will
/// be given as the **me** parameter to the HAL library's callback functions.
void uv_init(void *device);


#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
/// @brief: Deinitializes all modules and closes open sockets.
void uv_deinit(void);
#endif


/// @brief: Resets the non-volatile data from all HAL modules to defaults
void uv_data_reset();

extern bool rtos_init;
/// @brief: Returns true when the rtos HAL task is succesfully running and all peripherals should be
/// initialized.
static inline bool uv_rtos_initialized() {
	return rtos_init;
}


/// @brief: e.g. FreeRTOS applicationIdleHook, this function can be used
/// to add a idle task function. Idle function will be called every time
/// the application is performing the idle task loop.
///
/// @param task_function: The task function pointer. Takes user_ptr as a parameter.
/// Refer to uv_utilities.h for more details.
uv_errors_e uv_rtos_add_idle_task(void (*task_function)(void *user_ptr));

bool uv_rtos_idle_task_set(void);



extern uv_mutex_st halmutex;

/// @brief: Locks the HAL layer mutex for hal task functions.
///
/// @note: Only for HAL library inner use. Can be unlocked only from
/// other threads.
static inline void _uv_rtos_halmutex_lock(void) {
	uv_mutex_lock(&halmutex);
}

/// @brief: Unlocks the HAL layer mutex for hal task functions
///
/// @note: Only for HAL library inner use. Can be unlocked only from
/// other threads.
static inline void _uv_rtos_halmutex_unlock(void) {
	uv_mutex_unlock(&halmutex);
}


/// @brief: Wrapper for FreeRTOS vTaskDelaiUntil-function. Use this to
/// set task to wait for 'ms' milliseconds before continuing execution.
void uv_rtos_task_delay(unsigned int ms);


/// @brief: Forces a context switch
static inline void uv_rtos_task_yield(void) {
#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
	sched_yield();
#else
	taskYIELD();
#endif
}


#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
#define __NOP()
#endif




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
int32_t uv_rtos_task_create(void (*task_function)(void *this_ptr), char *task_name,
		unsigned int stack_depth, void *this_ptr,
		unsigned int task_priority, uv_rtos_task_ptr* handle);


/// @brief: Remove a task from the RTOS kernels management. The task being
/// deleted will be removed from all ready, blocked, suspended and event lists

#if !CONFIG_TARGET_LINUX && !CONFIG_TARGET_WIN
static inline void uv_rtos_task_delete(uv_rtos_task_ptr task) {
	vTaskDelete(task);
}



/// @brief: Returns the current tick timer count. Multiplying this value
/// with tick delay gives real world time.
static inline uint32_t uv_rtos_get_tick_count(void) {
	return xTaskGetTickCount();
}

static inline uint32_t uv_rtos_get_tick_rate_hz(void) {
	return configTICK_RATE_HZ;
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

#else


/// @brief: Starts the RTOS scheduler. After calling this, the
/// RTOS kernel has control over which tasks are executed and when.
/// In normal operation this function never returns.
void uv_rtos_start_scheduler(void);

/// @brief: Stops the RTOS scheduler. After calling this,
/// the RTOS stops functioning.
static inline void uv_rtos_end_scheduler(void) {
}

#endif





#endif /* CONFIG_RTOS_H_ */

