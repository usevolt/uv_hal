/*
 * uv_putchar.c
 *
 *  Created on: Apr 26, 2015
 *      Author: usevolt
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
static int8_t can_delay = 0;
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
	can_delay = CAN_DELAY_MS;

	// if CAN is in active state, wait until putting the message to the queue was succeeded.
	// otherwise just try to put it in queue. If the queue is full, message will be discarded.
	if (uv_can_get_error_state(CAN1) == CAN_ERROR_ACTIVE) {
		while (uv_can_send_message(CAN1, &msg) != ERR_NONE) {
			uv_rtos_task_delay(1);
		}
	}
	else {
		uv_can_send_message(CAN1, &msg);
	}

}
#endif


int outbyte(int c) {
	if (uv_rtos_initialized()) {
#if CONFIG_TERMINAL_UART

		uv_uart_send_char(UART0, c);

#endif
#if CONFIG_TERMINAL_CAN

		uint8_t ch = c;
		uv_vector_push_back(&can_vec, &ch);
		if (uv_vector_size(&can_vec) == uv_vector_max_size(&can_vec)) {
			send_can_msg();
		}

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
#if CONFIG_TERMINAL_CAN
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
