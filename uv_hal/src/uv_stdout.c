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

#include "uv_stdout.h"


#include "uv_rtos.h"
#if CONFIG_TERMINAL_CAN
#include "uv_can.h"
#include "uv_canopen.h"
#include "uv_terminal.h"
#include "uv_memory.h"
#include "uv_utilities.h"
#endif
#if CONFIG_TERMINAL_UART
#include "uv_uart.h"
#endif
#if CONFIG_TERMINAL_USBDVCOM
#include "cdc_vcom.h"
#endif




#if CONFIG_TERMINAL_CAN
#define CAN_DELAY_MS		4
static uint8_t can_buffer[4];
static uv_vector_st can_vec = UV_VECTOR_INIT(can_buffer, 4, 1);
#if !CONFIG_TARGET_LPC15XX && !CONFIG_TARGET_LPC40XX
static int8_t can_delay = 0;
#endif
#if CONFIG_TARGET_LPC40XX
// flags the characters waiting in can_vec were printed with
static unsigned int can_flags = 0u;
// serialises taking a message out of can_vec with queueing it, so that the HAL
// task and a printing task never put two messages on the bus out of order
static uv_mutex_st can_mutex;
static bool can_mutex_init = false;
// set once the HAL task runs: before that nothing empties the CAN transmit
// buffer, so characters are sent synchronously one by one, as they always were
static bool can_hal_running = false;
#endif
#endif

#if CONFIG_TERMINAL_CAN && !CONFIG_TARGET_LPC40XX
static void send_can_msg(unsigned int flags) {
	uv_can_message_st msg = {
			.data_length = 4 + uv_vector_size(&can_vec),
			.type = CAN_STD
	};
	msg.id = UV_TERMINAL_CAN_ID + uv_canopen_get_our_nodeid();
	uint8_t i;
	msg.data_8bit[0] = 0x42;
	msg.data_8bit[1] = UV_TERMINAL_CAN_INDEX & 0xFF;
	msg.data_8bit[2] = UV_TERMINAL_CAN_INDEX >> 8;
	msg.data_8bit[3] = UV_TERMINAL_CAN_SUBINDEX;
	for (i = 0; i < uv_vector_size(&can_vec); i++) {
		msg.data_8bit[4 + i] = *((uint8_t*)uv_vector_at(&can_vec, i));
	}
	uv_vector_clear(&can_vec);

#if !CONFIG_TARGET_LPC15XX && !CONFIG_TARGET_LPC40XX
	can_delay = CAN_DELAY_MS;

	// if CAN is in active state, wait until putting the message to the queue was succeeded.
	// otherwise just try to put it in queue. If the queue is full, message will be discarded.
	if (uv_can_get_error_state(CONFIG_CANOPEN_CHANNEL) == CAN_ERROR_ACTIVE) {
		while (uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg) != ERR_NONE) {
			uv_rtos_task_yield();
		}
	}
	else {
		uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
	}
#else
	uv_can_send_flags(CAN0, &msg, CAN_SEND_FLAGS_SYNC | flags);

#endif


}
#endif


#if CONFIG_TERMINAL_CAN && CONFIG_TARGET_LPC40XX
// A character goes out at once when the bus is free. While a message is still
// on its way, the characters that follow gather into the next one, up to its
// four data bytes, and it is queued when full or when the bus frees up (seen by
// the next character or by the HAL step). Nothing waits on a timer, and each
// message is final by the time it is queued, which is when the CAN tx callback
// hands it on to whatever forwards the terminal further.

static void can_msg_init(uv_can_message_st *msg, uint8_t count) {
	msg->type = CAN_STD;
	msg->id = UV_TERMINAL_CAN_ID + uv_canopen_get_our_nodeid();
	msg->data_length = 4u + count;
	msg->data_8bit[0] = 0x42;
	msg->data_8bit[1] = UV_TERMINAL_CAN_INDEX & 0xFF;
	msg->data_8bit[2] = UV_TERMINAL_CAN_INDEX >> 8;
	msg->data_8bit[3] = UV_TERMINAL_CAN_SUBINDEX;
}


/// @brief: Takes the characters waiting in can_vec as one terminal message.
/// Returns false when there were none.
static bool can_take(uv_can_message_st *msg, unsigned int *flags) {
	bool ret = false;
	uv_disable_int();
	uint8_t count = (uint8_t) uv_vector_size(&can_vec);
	if (count != 0u) {
		can_msg_init(msg, count);
		for (uint8_t i = 0u; i < count; i++) {
			msg->data_8bit[4u + i] = *((uint8_t*) uv_vector_at(&can_vec, i));
		}
		uv_vector_clear(&can_vec);
		*flags = can_flags;
		ret = true;
	}
	else {
	}
	uv_enable_int();
	return ret;
}


/// @brief: Queues a terminal message. Called with can_mutex held.
static void can_send(uv_can_message_st *msg, unsigned int flags) {
	// A full transmit buffer is waited out rather than overrun, unless the bus
	// is in error and would never drain it. The message is sent only once:
	// every attempt runs the tx callback, which forwards it.
	while (uv_can_tx_full(CAN0) &&
			(uv_can_get_error_state(CAN0) == CAN_ERROR_ACTIVE)) {
		uv_rtos_task_delay(1);
	}
	(void) uv_can_send_flags(CAN0, msg, CAN_SEND_FLAGS_NORMAL | flags);
}


static void can_putc(unsigned int flags, uint8_t c) {
	if (!can_hal_running || (xPortIsInsideInterrupt() != pdFALSE)) {
		// Nothing empties the transmit buffer yet, or this is an interrupt,
		// which must not wait on a mutex: the character goes on its own
		uv_can_message_st msg = { };
		can_msg_init(&msg, 1u);
		msg.data_8bit[4] = c;
		(void) uv_can_send_flags(CAN0, &msg, CAN_SEND_FLAGS_SYNC | flags);
	}
	else {
		if (!can_mutex_init) {
			uv_mutex_init(&can_mutex);
			uv_mutex_unlock(&can_mutex);
			can_mutex_init = true;
		}
		else {
		}
		(void) uv_mutex_lock(&can_mutex);
		uv_can_message_st msg = { };
		unsigned int msg_flags = 0u;
		// One message carries one set of flags, so characters that are not to
		// be forwarded never share a message with ones that are
		if ((flags != can_flags) && can_take(&msg, &msg_flags)) {
			can_send(&msg, msg_flags);
		}
		else {
		}
		bool idle = uv_can_tx_idle(CAN0);
		uv_disable_int();
		uv_vector_push_back(&can_vec, &c);
		can_flags = flags;
		bool full = (uv_vector_size(&can_vec) == uv_vector_max_size(&can_vec));
		uv_enable_int();
		if ((idle || full) && can_take(&msg, &msg_flags)) {
			can_send(&msg, msg_flags);
		}
		else {
		}
		uv_mutex_unlock(&can_mutex);
	}
}


static void can_step(void) {
	can_hal_running = true;
	// Only ever tries the mutex: the HAL task must not wait on a printing task.
	// A message it cannot take now is taken by the next character or step.
	if (can_mutex_init &&
			(uv_vector_size(&can_vec) != 0) &&
			uv_can_tx_idle(CAN0) &&
			(xSemaphoreTake(can_mutex, 0) == pdTRUE)) {
		uv_can_message_st msg = { };
		unsigned int msg_flags = 0u;
		if (can_take(&msg, &msg_flags)) {
			can_send(&msg, msg_flags);
		}
		else {
		}
		uv_mutex_unlock(&can_mutex);
	}
	else {
	}
}
#endif


int outbyte(unsigned int flags, int c) {
#if CONFIG_TERMINAL
#if CONFIG_TERMINAL_UART

		if (uv_active_terminal() == TERMINAL_UART) {
			uv_uart_send_char(UART0, c);
		}

#endif
#if CONFIG_TERMINAL_CAN
		if (uv_active_terminal() == TERMINAL_CAN) {
			uint8_t ch = c;
#if CONFIG_TARGET_LPC40XX
			can_putc(flags, ch);
#else
			uv_vector_push_back(&can_vec, &ch);
#if !CONFIG_TARGET_LPC15XX
			if (uv_vector_size(&can_vec) == uv_vector_max_size(&can_vec)) {
				send_can_msg(flags);
			}
#else
			send_can_msg(flags);
#endif
#endif
		}
#endif
#if CONFIG_TERMINAL_USBDVCOM
		if (uv_active_terminal() == TERMINAL_USB) {
			if (vcom_connected()) {
				vcom_write((char*) &c, 1);
			}
		}
#endif

#endif
	return 1;
}



void uv_stdout_send(char* str, unsigned int count) {
	int i;
	for (i = 0; i < count; i++) {
		outbyte(PRINTF_FLAGS_NONE, str[i]);
	}
}


void _uv_stdout_hal_step(unsigned int step_ms) {
#if (CONFIG_TERMINAL_CAN && CONFIG_TARGET_LPC40XX)
	(void) step_ms;
	can_step();
#elif (CONFIG_TERMINAL_CAN && (!CONFIG_TARGET_LPC15XX))
	if (uv_vector_size(&can_vec) != 0) {
		if (can_delay > 0) {
			can_delay -= step_ms;
		}
		else {
			send_can_msg(PRINTF_FLAGS_NONE);
		}
	}
#endif
}
