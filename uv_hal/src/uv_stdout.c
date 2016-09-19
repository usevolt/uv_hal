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





void uv_stdout_send(char* str, unsigned int count) {
	int i;
	for (i = 0; i < count; i++) {
		putchar(str[i]);
	}
}
