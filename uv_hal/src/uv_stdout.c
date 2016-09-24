/*
 * uv_putchar.c
 *
 *  Created on: Apr 26, 2015
 *      Author: usevolt
 */

#include "uv_stdout.h"


#include "uv_can.h"
#include "uv_uart.h"



static uint8_t stdout = STDOUT_UART0;

inline void uv_stdout_set_source(uv_stdout_sources_e value) {
	stdout = value;
}

int outbyte(int c) {
	if (uv_uart_is_initialized(UART0)) {
		uv_uart_send_char(UART0, c);
	}
	return 1;
}



void uv_stdout_send(char* str, unsigned int count) {
	int i;
	for (i = 0; i < count; i++) {
		outbyte(str[i]);
	}
}
