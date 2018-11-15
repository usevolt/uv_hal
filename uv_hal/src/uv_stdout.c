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

#include "uv_stdout.h"


#include "uv_rtos.h"
#if CONFIG_TERMINAL_CAN
#include "uv_can.h"
#include "uv_terminal.h"
#include "uv_memory.h"
#include "uv_utilities.h"
#endif
#if CONFIG_TERMINAL_UART
#include "uv_uart.h"
#endif



#if CONFIG_TERMINAL_CAN
#define CAN_DELAY_MS		4
static uint8_t can_buffer[4];
static uv_vector_st can_vec = UV_VECTOR_INIT(can_buffer, 4, 1);
#if !CONFIG_TARGET_LPC1549
static int8_t can_delay = 0;
#endif
#endif

#if CONFIG_TERMINAL_CAN
static void send_can_msg(void) {
	uv_can_message_st msg = {
			.id = UV_TERMINAL_CAN_ID + uv_get_id(),
			.data_length = 4 + uv_vector_size(&can_vec),
			.type = CAN_STD
	};
	uint8_t i;
	msg.data_8bit[0] = 0x42;
	msg.data_8bit[1] = UV_TERMINAL_CAN_INDEX & 0xFF;
	msg.data_8bit[2] = UV_TERMINAL_CAN_INDEX >> 8;
	msg.data_8bit[3] = UV_TERMINAL_CAN_SUBINDEX;
	for (i = 0; i < uv_vector_size(&can_vec); i++) {
		msg.data_8bit[4 + i] = *((uint8_t*)uv_vector_at(&can_vec, i));
	}
	uv_vector_clear(&can_vec);
#if !CONFIG_TARGET_LPC1549
	can_delay = CAN_DELAY_MS;

	// if CAN is in active state, wait until putting the message to the queue was succeeded.
	// otherwise just try to put it in queue. If the queue is full, message will be discarded.
	if (uv_can_get_error_state(CAN0) == CAN_ERROR_ACTIVE) {
		while (uv_can_send_message(CAN0, &msg) != ERR_NONE) {
			uv_rtos_task_yield();
		}
	}
	else {
		uv_can_send_message(CAN0, &msg);
	}
#else
	uv_can_send_sync(CAN0, &msg);
#endif

}
#endif


int outbyte(int c) {
	if (uv_rtos_initialized() && uv_terminal_enabled) {
#if CONFIG_TERMINAL_UART

		uv_uart_send_char(UART0, c);

#endif
#if CONFIG_TERMINAL_CAN

		uint8_t ch = c;
		uv_vector_push_back(&can_vec, &ch);
#if !CONFIG_TARGET_LPC1549
		if (uv_vector_size(&can_vec) == uv_vector_max_size(&can_vec)) {
			send_can_msg();
		}
#else
		send_can_msg();
#endif

#endif
	}
	return 1;
}



void uv_stdout_send(char* str, unsigned int count) {
	int i;
	for (i = 0; i < count; i++) {
		outbyte(str[i]);
	}
}


void _uv_stdout_hal_step(unsigned int step_ms) {
#if (CONFIG_TERMINAL_CAN && (!CONFIG_TARGET_LPC1549))
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
