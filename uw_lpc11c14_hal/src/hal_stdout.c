/*
 * hal_putchar.c
 *
 *  Created on: Apr 26, 2015
 *      Author: usevolt
 */

#include "hal_stdout.h"
#include "hal_can_controller.h"
#include "hal_uart_controller.h"



static uint8_t stdout = STDOUT_UART;

inline void hal_stdout_set_source(hal_stdout_sources_e value) {
	stdout = value;
}


static uint8_t byte_count = 0;

void putchar(const char c)
{
	switch (stdout) {
	case STDOUT_UART:
		hal_uart0_send_char(c);
		break;
	case STDOUT_CAN:
//		hal_canopen_pdo_msg[CANOPEN_TXPDO4].data[byte_count++] = c;
		if (byte_count > 8 || c == '\n' || c == '\r') {
//			hal_canopen_pdo_msg[CANOPEN_TXPDO4].data_length = byte_count;
//			hal_canopen_send_pdo(CANOPEN_TXPDO4);
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




void hal_stdout_send(char* str, unsigned int count) {
	int i;
	for (i = 0; i < count; i++) {
		putchar(str[i]);
	}
}
