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
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"
#include "semphr.h"
#include <uv_hal_config.h>

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
#if defined(CONFIG_UV_BOOTLOADER)
// defines the start address where the firmware is found.
// Usually defaults to 0x0, but uv_bootloader changes this to 0x1000
// since the bootloader resides in 0x0.
// NOTE: On LPC4078 bootloader consmues 2 sectors of flash, thus this is 0x2000
#if CONFIG_TARGET_LPC40XX
#define APP_START_ADDR						0x2000
#else
#define APP_START_ADDR						0x1000
#endif
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
#if (CONFIG_TARGET_LPC15XX || CONFIG_TARGET_LPC40XX)
#define uv_disable_int()		__disable_irq()
#elif CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
#define uv_disable_int()
#endif
/// @brief: Disabled all interrupts. This is meant to be called from
/// inside interrupt routines.
#define uv_disable_int_ISR()	taskENTER_CRITICAL_FROM_ISR()
/// @brief: Enables all interrupts. This shouldn't be called from
/// interrupt routines, use uv_enable_int_ISR instead!
#if (CONFIG_TARGET_LPC15XX || CONFIG_TARGET_LPC40XX)
#define uv_enable_int()			__enable_irq()
#elif CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
#define uv_enable_int()
#endif
/// @brief: Enabled all interrupts. This is meant to be called from
/// inside interrupt routines.
#define uv_enable_int_ISR()		taskEXIT_CRITICAL_FROM_ISR(1)

#if (CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN)
#define uv_enter_critical()
#else
#define uv_enter_critical()		taskENTER_CRITICAL()
#endif

#if (CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN)
#define uv_exit_critical()
#else
#define uv_exit_critical()		taskEXIT_CRITICAL()
#endif



typedef xTaskHandle 		uv_rtos_task_ptr;


/// @brief: Wrapper for FreeRTOS mutex semaphore
typedef SemaphoreHandle_t 	uv_mutex_st;

/// @brief: Initializes a mutex. Must be called before uv_mutex_lock and uv_mutex_unlock
static inline void uv_mutex_init(uv_mutex_st *mutex) {
	*mutex = xSemaphoreCreateBinary();
	xSemaphoreGive(*mutex);
}

#include <stdio.h>
/// @brief: Locks the mutex. No one else can lock the mutex before it is unlocked.
static inline bool uv_mutex_lock(uv_mutex_st *mutex) {
	return xSemaphoreTake(*mutex, portMAX_DELAY);
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
static inline uv_queue_st uv_queue_init(uv_queue_st *this,
		int32_t queue_len, uint32_t element_size) {
	*this = xQueueCreate(queue_len, element_size);
	return *this;
}

/// @brief: Deletes a queue and frees all memory allocated for it
static inline void uv_queue_delete(uv_queue_st *this) {
	vQueueDelete(*this);
}

/// @brief: Empties the queue
static inline void uv_queue_clear(uv_queue_st *this) {
	xQueueReset(*this);
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


typedef StreamBufferHandle_t uv_streambuffer_st;

/// @brief: Initializes the streambuffer
static inline void uv_streambuffer_init(uv_streambuffer_st *this, uint32_t len_bytes) {
	*this = xStreamBufferCreate(len_bytes, 1);
}

/// @brief: Returns the number of data currently in stream buffer, in bytes
static inline uint32_t uv_streambuffer_get_len(uv_streambuffer_st *this) {
	return xStreamBufferBytesAvailable(*this);
}

/// @brief: Returns the free data space available in the stream buffer
static inline uint32_t uv_streambuffer_get_free_space(uv_streambuffer_st *this) {
	return xStreamBufferSpacesAvailable(*this);
}

static inline int32_t uv_streambuffer_get_max_len(uv_streambuffer_st *this) {
	return xStreamBufferBytesAvailable(*this) + xStreamBufferSpacesAvailable(*this);
}

/// @return: true if streambuffer is currently empty, false otherwise
static inline bool uv_streambuffer_is_empty(uv_streambuffer_st *this) {
	return xStreamBufferIsEmpty(*this);
}

/// @brief: Pushes new data to stream buffer
static inline uint32_t uv_streambuffer_push(uv_streambuffer_st *this,
		void *data, uint32_t len, int32_t wait_ms) {
	return xStreamBufferSend(*this, data, len, wait_ms);
}


/// @brief: Puses new data into the stream buffef. Only to be used inside ISR
static inline uint32_t uv_streambuffer_push_isr(uv_streambuffer_st *this,
		void *data, uint32_t len) {
	return xStreamBufferSendFromISR(*this, data, len, NULL);
}

/// @brief: Pops data out from streambuffer in FIFO manner
static inline uint32_t uv_streambuffer_pop(uv_streambuffer_st *this,
		void *data, uint32_t len, int32_t wait_ms) {
	return xStreamBufferReceive(*this, data, len, wait_ms);
}

/// @brief: Pops data out from streambuffer in FIFO manner, only to be used in ISRs
static inline uint32_t uv_streambuffer_pop_isr(uv_streambuffer_st *this,
		void *data, uint32_t len) {
	return xStreamBufferReceiveFromISR(*this, data, len, NULL);
}

/// @brief: Clears and resets all data stored in the stream
static inline void uv_streambuffer_clear(uv_streambuffer_st *this) {
	xStreamBufferReset(*this);
}





/// @brief: Initializes the real time OS. Basicly sets up a HAL task which takes care
/// of several hal module step functions
///
/// @param device: Pointer to the main application data structure. This is the struct which will
/// be given as the **me** parameter to the HAL library's callback functions.
void uv_init(void *device);



#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
/// @brief: Initializes the real time OS with argc and argv. This is implemented only
/// on computer targets
void uv_init_arg(void *device, int argc, char *argv[]);



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
	taskYIELD();
}


#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
#define __NOP()
#endif




/// @brief: Create a new task and add it to the list of tasks that are ready to run.
///
/// @param task_function: The function which will be executed as a task
/// @param task_name: A name for the task. The maximum length for the name is
/// defined in the FreeRTOSConfig.h file.
/// @param stack_depth: The maximum size for the task's stack, defined in WORDS
/// @param this_ptr: A user definable pointer which will be passed to the task function.
/// @param task_priority: The priority of the task. Bigger value means higher priority.
/// @param handle: A optional return handle for the created task. Will be set to NULL
/// if task creation failed.
int32_t uv_rtos_task_create(void (*task_function)(void *this_ptr), char *task_name,
		unsigned int stack_depth, void *this_ptr,
		unsigned int task_priority, uv_rtos_task_ptr* handle);


/// @brief: Remove a task from the RTOS kernels management. The task being
/// deleted will be removed from all ready, blocked, suspended and event lists
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
void uv_rtos_start_scheduler(void);

/// @brief: Stops the RTOS scheduler. After calling this,
/// the RTOS stops functioning.
static inline void uv_rtos_end_scheduler(void) {
	vTaskEndScheduler();
}



/// @brief: Resets the HAL layer non-volatile settings to default values
///
/// @note: Only for HAL library's internal use
void _uv_rtos_hal_reset(void);



#endif /* CONFIG_RTOS_H_ */

