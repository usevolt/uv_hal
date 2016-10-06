/*
 * uv_putchar.c
 *
 *  Created on: Apr 26, 2015
 *      Author: usevolt
 */

#include "uv_stdout.h"


#if CONFIG_TERMINAL_CAN
#include "uv_can.h"
#include "uv_terminal.h"
#include "uv_memory.h"
#endif
#if CONFIG_TERMINAL_UART
#include "uv_uart.h"
#endif






int outbyte(int c) {
#if CONFIG_TERMINAL_UART
	if (uv_uart_is_initialized(UART0)) {
		uv_uart_send_char(UART0, c);
	}
#endif
#if CONFIG_TERMINAL_CAN
	uv_can_message_st msg = {
			.id = UV_TERMINAL_CAN_PREFIX + uv_get_crc(),
			.data_length = 1,
			.data_8bit[0] = c
	};
	while (uv_can_send_message(CAN1, &msg));
#endif
	return 1;
}



void uv_stdout_send(char* str, unsigned int count) {
	int i;
	for (i = 0; i < count; i++) {
		outbyte(str[i]);
	}
}
