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
#if CONFIG_TARGET_LPC15XX
#include "chip.h"
#include "uart_15xx.h"
#include "swm_15xx.h"
#include "iocon_15xx.h"
#endif
#include "uv_memory.h"
#include "uv_utilities.h"
#if CONFIG_RTOS
#include "uv_rtos.h"
#endif





typedef struct {
	void (*callback[UART_COUNT])(void*, uv_uarts_e);
	uv_streambuffer_st buffer[UART_COUNT];
} uart_st;




static uart_st _this = {};

#define this (&_this)


//UART interrupt handlers
#if CONFIG_TARGET_LPC15XX
#if CONFIG_UART0 || CONFIG_UART1 || CONFIG_UART2
static void isr(uv_uarts_e uart) {
	char c;
	if (Chip_UART_Read((void*) uart, &c, 1)) {
		uint8_t i = 0;
#if CONFIG_UART0
		if (uart == UART0) { i = 0; }
#endif
#if CONFIG_UART1
		if (uart == UART1) { i = 1; }
#endif
#if CONFIG_UART2
		if (uart == UART2) { i = 2; }
#endif
		int32_t e = uv_streambuffer_push_isr(&this->buffer[i], &c, 1);
		if (!e) {
			uv_log_error(e);
		}
		else {
			printf("%02x ", c);
		}
		if (this->callback[i]) {
			this->callback[i](__uv_get_user_ptr(), uart);
		}
	}
}
#endif
#if CONFIG_UART0
void UART0_IRQHandler(void) {
	isr(UART0);
}
#endif
#if CONFIG_UART1
void UART1_IRQHandler(void) {
	isr(UART1);
}
#endif
#if CONFIG_UART2
void UART2_IRQHandler(void) {
	isr(UART2);
}
#endif

#endif




void uv_uart_add_callback(uv_uarts_e uart,
		void (*callback_function)(void* user_ptr, uv_uarts_e uart)) {
#if CONFIG_TARGET_LPC15XX
#if CONFIG_UART0
	if (uart == UART0) { uart = 0; }
#endif
#if CONFIG_UART1
	if (uart == UART1) { uart = 1; }
#endif
#if CONFIG_UART2
	if (uart == UART2) { uart = 2; }
#endif
#endif
	this->callback[uart] = callback_function;
}


#if CONFIG_TARGET_LPC15XX
uv_errors_e _uv_uart_init(uv_uarts_e uart) {
	uv_errors_e ret = ERR_NONE;

	Chip_Clock_SetUARTBaseClockRate(Chip_Clock_GetMainClockRate(), true);

	#if CONFIG_UART0
	uint32_t baud;
	if (uart == UART0) {
		baud = CONFIG_UART0_BAUDRATE;
		ret = uv_streambuffer_init(&this->buffer[0], CONFIG_UART0_RX_BUFFER_SIZE);
		Chip_IOCON_PinMuxSet(LPC_IOCON, uv_gpio_get_port(CONFIG_UART0_TX_PIN),
				uv_gpio_get_pin(CONFIG_UART0_TX_PIN),
				(IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_IOCON_PinMuxSet(LPC_IOCON, uv_gpio_get_port(CONFIG_UART0_RX_PIN),
				uv_gpio_get_pin(CONFIG_UART0_RX_PIN),
				(IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));

		Chip_SWM_MovablePortPinAssign(SWM_UART0_TXD_O,
				uv_gpio_get_port(CONFIG_UART0_TX_PIN),
				uv_gpio_get_pin(CONFIG_UART0_TX_PIN));
		Chip_SWM_MovablePortPinAssign(SWM_UART0_RXD_I,
				uv_gpio_get_port(CONFIG_UART0_RX_PIN),
				uv_gpio_get_pin(CONFIG_UART0_RX_PIN));
		NVIC_EnableIRQ(UART0_IRQn);

	}
#endif
#if CONFIG_UART1
	if (uart == UART1) {
		baud = CONFIG_UART1_BAUDRATE;
		uv_ring_buffer_init(&this->buffer[1], uart1_rxbuffer, CONFIG_UART1_RX_BUFFER_SIZE, sizeof(char));
		Chip_IOCON_PinMuxSet(LPC_IOCON, uv_gpio_port(CONFIG_UART1_TX_PIN),
				uv_gpio_pin(CONFIG_UART1_TX_PIN), (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_IOCON_PinMuxSet(LPC_IOCON, uv_gpio_port(CONFIG_UART1_RX_PIN),
				uv_gpio_pin(CONFIG_UART1_RX_PIN), (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));

		Chip_SWM_MovablePortPinAssign(SWM_UART1_TXD_O,
				uv_gpio_port(CONFIG_UART1_TX_PIN), uv_gpio_pin(CONFIG_UART1_TX_PIN));
		Chip_SWM_MovablePortPinAssign(SWM_UART1_RXD_I,
				uv_gpio_port(CONFIG_UART1_RX_PIN), uv_gpio_pin(CONFIG_UART1_RX_PIN));
		NVIC_EnableIRQ(UART1_IRQn);
	}
#endif
#if CONFIG_UART2
	if (uart == UART2) {
		baud = CONFIG_UART2_BAUDRATE;
		uv_ring_buffer_init(&this->buffer[2], uart2_rxbuffer, CONFIG_UART2_RX_BUFFER_SIZE, sizeof(char));
		Chip_IOCON_PinMuxSet(LPC_IOCON, uv_gpio_port(CONFIG_UART2_TX_PIN),
				uv_gpio_pin(CONFIG_UART2_TX_PIN), (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_IOCON_PinMuxSet(LPC_IOCON, uv_gpio_port(CONFIG_UART2_RX_PIN),
				uv_gpio_pin(CONFIG_UART2_RX_PIN), (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));

		Chip_SWM_MovablePortPinAssign(SWM_UART2_TXD_O,
				uv_gpio_port(CONFIG_UART2_TX_PIN), uv_gpio_pin(CONFIG_UART2_TX_PIN));
		Chip_SWM_MovablePortPinAssign(SWM_UART2_RXD_I,
				uv_gpio_port(CONFIG_UART2_RX_PIN), uv_gpio_pin(CONFIG_UART2_RX_PIN));
		NVIC_EnableIRQ(UART2_IRQn);
	}
#endif
	this->callback[0] = NULL;
	this->callback[1] = NULL;
	this->callback[2] = NULL;

	Chip_UART_Init((void*) uart);
	Chip_UART_ConfigData((void*) uart,
			UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1 | UART_CFG_DATALEN_8);
	Chip_UART_SetBaud((void*) uart, baud);
	Chip_UART_TXEnable((void*) uart);
	Chip_UART_IntEnable((void*) uart, UART_INTEN_RXRDY);

	Chip_UART_Enable((void*) uart);

	return ret;
}

#endif




uv_errors_e uv_uart_send_char(uv_uarts_e uart, char buffer) {
	uv_errors_e ret = ERR_NONE;

#if CONFIG_TARGET_LPC15XX
	Chip_UART_SendBlocking((void *) uart, &buffer, 1);
#endif

	return ret;
}



uv_errors_e uv_uart_send(uv_uarts_e uart, char *buffer, uint32_t length) {

	printf("uart writing ");
	for (uint16_t i = 0; i < length; i++) {
		printf("0x%02x ", buffer[i]);
	}
	printf("\n");
	while (length != 0) {
		uv_uart_send_char(uart, *buffer);
		buffer++;
		length--;
	}
	return ERR_NONE;
}



uv_errors_e uv_uart_send_str(uv_uarts_e uart, char *buffer) {
	while (*buffer != '\0') {
		uv_uart_send_char(uart, *buffer);

		buffer++;
	}

	return ERR_NONE;
}


int32_t uv_uart_get(uv_uarts_e uart, char *dest, uint32_t max_len, int wait_ms) {
	int32_t ret = 0;

	uint32_t i = 0;
#if CONFIG_TARGET_LPC15XX
#if CONFIG_UART0
	if (uart == UART0) { i = 0; }
#endif
#if CONFIG_UART1
	if (uart == UART1) { i = 1; }
#endif
#if CONFIG_UART2
	if (uart == UART2) { i = 2; }
#endif
#endif
	ret = uv_streambuffer_pop(&this->buffer[i], dest, max_len, wait_ms);
	return ret;
}



bool uv_uart_receive_cmp(uv_uarts_e uart, char *str, uint32_t max_len, int wait_ms) {
	bool ret = false;
	int32_t i = 0;
	char c;
	while (uv_uart_get(uart, &c, 1, wait_ms)) {
		if (c == str[i]) {
			i++;
			if (i == max_len) {
				ret = true;
				break;
			}
		}
		else {
			break;
		}
	}
	return ret;
}


void uv_uart_clear_rx_buffer(uv_uarts_e uart) {
	uint32_t i = uart;
#if CONFIG_UART0
	if (uart == UART0) { i = 0; }
#endif
#if CONFIG_UART1
	if (uart == UART1) { i = 1; }
#endif
#if CONFIG_UART2
	if (uart == UART2) { i = 2; }
#endif
	uv_streambuffer_clear(&this->buffer[i]);

}


#endif
