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

#include "uv_uart.h"


#if CONFIG_UART

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "uv_gpio.h"
#include "uv_memory.h"
#include "uv_utilities.h"
#if CONFIG_RTOS
#include "uv_rtos.h"
#endif










void uv_uart_add_callback(uv_uarts_e uart,
		void (*callback_function)(void* user_ptr, uv_uarts_e uart)) {
}


uv_errors_e _uv_uart_init(uv_uarts_e uart) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}





uv_errors_e uv_uart_send_char(uv_uarts_e uart, char buffer) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}



int32_t uv_uart_send(uv_uarts_e uart, char *buffer, uint32_t length) {
	return length;
}



uv_errors_e uv_uart_send_str(uv_uarts_e uart, char *buffer) {
	return ERR_NONE;
}


int32_t uv_uart_get_tx_free_space(uv_uarts_e uart) {
	return INT32_MAX;
}


void uv_uart_set_baudrate(uv_uarts_e uart, unsigned int baudrate) {

}


int32_t uv_uart_get(uv_uarts_e uart, char *dest, uint32_t max_len, int wait_ms) {
	int32_t ret = 0;

	return ret;
}



bool uv_uart_receive_cmp(uv_uarts_e uart, char *str, uint32_t max_len, int wait_ms) {
	bool ret = false;
	return ret;
}


void uv_uart_clear_rx_buffer(uv_uarts_e uart) {

}



void uv_uart_break_start(uv_uarts_e uart) {
}

void uv_uart_break_stop(uv_uarts_e uart) {
}


#endif
