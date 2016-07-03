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


static uint8_t byte_count = 0;



void putchar(const char c)
{

	switch (stdout) {
	case STDOUT_UART0:
		if (uv_uart_is_initialized(UART0)) {
			uv_uart_send_char(UART0, c);
		}
		break;
	case STDOUT_CAN:
//		uv_canopen_pdo_msg[CANOPEN_TXPDO4].data[byte_count++] = c;
		if (byte_count > 8 || c == '\n' || c == '\r') {
//			uv_canopen_pdo_msg[CANOPEN_TXPDO4].data_length = byte_count;
//			uv_canopen_send_pdo(CANOPEN_TXPDO4);
			byte_count = 0;
		}
		break;
	default:
		return;
	}
}

int puts(const char * str)
{
  while(*str) putchar(*str++);

  return 0;
}




void uv_stdout_send(char* str, unsigned int count) {
	int i;
	for (i = 0; i < count; i++) {
		putchar(str[i]);
	}
}
