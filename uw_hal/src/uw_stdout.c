/*
 * uw_putchar.c
 *
 *  Created on: Apr 26, 2015
 *      Author: usevolt
 */

#include <uw_stdout.h>

#include "uw_can.h"
#include "uw_uart.h"



static uint8_t stdout = STDOUT_UART;

inline void uw_stdout_set_source(uw_stdout_sources_e value) {
	stdout = value;
}


static uint8_t byte_count = 0;

void putchar(const char c)
{

	switch (stdout) {
	case STDOUT_UART:
		if (uw_uart_is_initialized(UART0)) {
			uw_uart_send_char(UART0, c);
		}
		break;
	case STDOUT_CAN:
//		uw_canopen_pdo_msg[CANOPEN_TXPDO4].data[byte_count++] = c;
		if (byte_count > 8 || c == '\n' || c == '\r') {
//			uw_canopen_pdo_msg[CANOPEN_TXPDO4].data_length = byte_count;
//			uw_canopen_send_pdo(CANOPEN_TXPDO4);
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




void uw_stdout_send(char* str, unsigned int count) {
	int i;
	for (i = 0; i < count; i++) {
		putchar(str[i]);
	}
}
