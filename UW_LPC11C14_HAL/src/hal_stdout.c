/*
 * hal_putchar.c
 *
 *  Created on: Apr 26, 2015
 *      Author: usevolt
 */

#include "hal_stdout.h"
#include "hal_can_controller.h"
#include "hal_uart_controller.h"



static hal_stdout_e stdout = HAL_STDOUT_UART;

inline void hal_set_stdout(hal_stdout_e value) {
	stdout = value;
}


//references to hal_can_controller.c
extern hal_can_msg_obj_st hal_canopen_pdo_msg[CANOPEN_PDO_COUNT];
static uint8_t byte_count = 0;

void putchar(const char c)
{
	switch (stdout) {
	case HAL_STDOUT_UART:
		hal_uart0_send_char(c);
		break;
	case HAL_STDOUT_CAN:
		hal_canopen_pdo_msg[CANOPEN_TXPDO4].data[byte_count++] = c;
		if (byte_count > 8 || c == '\n' || c == '\r') {
			hal_canopen_pdo_msg[CANOPEN_TXPDO4].data_length = byte_count;
			hal_canopen_send_pdo(CANOPEN_TXPDO4);
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

