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

#include <stdint.h>
#include <FreeRTOS.h>
#include <queue.h>

/// @file: The complete set of RTOS symbols the modules under test reference at
/// link time. Keeping this list short is intentional and is a useful signal in
/// itself: if a module cannot be unit tested without stubbing out a growing pile
/// of the RTOS, it is a sign the module is doing more than pure logic.
///
/// The tests never start the FreeRTOS scheduler, so the tick counter is simply
/// a monotonic counter. No test currently depends on its value.


static TickType_t fake_ticks = 0;


TickType_t xTaskGetTickCount(void) {
	fake_ticks++;
	return fake_ticks;
}


/// The uv_mutex_* helpers in uv_rtos.h are inline wrappers around FreeRTOS
/// binary semaphores, which are queues underneath. The tests are single
/// threaded, so a mutex never contends: creating one hands out a dummy
/// non-NULL handle and every give and take succeeds.
static uint8_t fake_queue;


QueueHandle_t xQueueGenericCreate(const UBaseType_t uxQueueLength,
		const UBaseType_t uxItemSize, const uint8_t ucQueueType) {
	return (QueueHandle_t) &fake_queue;
}


BaseType_t xQueueGenericSend(QueueHandle_t xQueue, const void * const pvItemToQueue,
		TickType_t xTicksToWait, const BaseType_t xCopyPosition) {
	return pdPASS;
}


BaseType_t xQueueSemaphoreTake(QueueHandle_t xQueue, TickType_t xTicksToWait) {
	return pdPASS;
}
