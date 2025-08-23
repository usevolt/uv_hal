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



uint32_t printf_flags = 0;


#if CONFIG_TERMINAL_CAN
#define CAN_DELAY_MS		4
static uint8_t can_buffer[4];
static uv_vector_st can_vec = UV_VECTOR_INIT(can_buffer, 4, 1);
#if !CONFIG_TARGET_LPC15XX
static int8_t can_delay = 0;
#endif
#endif

#if CONFIG_TERMINAL_CAN
static void send_can_msg(void) {
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
	can_send_flags_e flags = CAN_SEND_FLAGS_SYNC;
	if (printf_flags & PRINTF_FLAGS_NOTXCALLB) {
		flags |= CAN_SEND_FLAGS_NO_TX_CALLB;
	}
	uv_can_send_flags(CAN0, &msg, flags);

#endif


}
#endif


int outbyte(int c) {
#if CONFIG_TERMINAL
#if CONFIG_TERMINAL_UART

		if (uv_active_terminal() == TERMINAL_UART) {
			uv_uart_send_char(UART0, c);
		}

#endif
#if CONFIG_TERMINAL_CAN
		if (uv_active_terminal() == TERMINAL_CAN) {
			uint8_t ch = c;
			uv_vector_push_back(&can_vec, &ch);
#if !CONFIG_TARGET_LPC15XX && !CONFIG_TARGET_LPC40XX
			if (uv_vector_size(&can_vec) == uv_vector_max_size(&can_vec)) {
				send_can_msg();
			}
#else
			send_can_msg();
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
		outbyte(str[i]);
	}
}


void _uv_stdout_hal_step(unsigned int step_ms) {
#if (CONFIG_TERMINAL_CAN && (!CONFIG_TARGET_LPC15XX))
	if (uv_vector_size(&can_vec) != 0) {
		if (can_delay > 0) {
			can_delay -= step_ms;
		}
		else {
			send_can_msg();
		}
	}
#endif
}
