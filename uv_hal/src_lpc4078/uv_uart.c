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
#include "chip_lpc407x_8x.h"
#include "uv_gpio.h"
#include "uart_17xx_40xx.h"
#endif
#include "uv_memory.h"
#include "uv_utilities.h"
#if CONFIG_RTOS
#include "uv_rtos.h"
#endif





typedef struct {
	void (*callback)(void*, uv_uarts_e);
	uv_streambuffer_st rx_buffer;
	uv_streambuffer_st tx_buffer;
	LPC_USART_T *uart;
} uart_st;

#if CONFIG_UART0
static uart_st uart0 = {
		.uart = LPC_UART0,
		.callback = NULL
};
#endif
#if CONFIG_UART1
static uart_st uart1 = {
		.uart = LPC_UART1,
		.callback = NULL
};
#endif
#if CONFIG_UART2
static uart_st uart2 = {
		.uart = LPC_UART2,
		.callback = NULL
};
#endif
#if CONFIG_UART3
static uart_st uart3 = {
		.uart = LPC_UART3,
		.callback = NULL
};
#endif



static uart_st* u[UART_COUNT] = {
#if CONFIG_UART0
		&uart0,
#else
		NULL,
#endif
#if CONFIG_UART1
		&uart1,
#else
		NULL,
#endif
#if CONFIG_UART2
		&uart2,
#else
		NULL,
#endif
#if CONFIG_UART3
		&uart3
#else
		NULL
#endif
};

#define this (u[uart])


//UART interrupt handlers
#if CONFIG_UART0
void UART0_IRQHandler(void) {
	char c;
	if (Chip_UART_ReadLineStatus(u[0]->uart) & UART_LSR_RDR) {
		if (Chip_UART_Read(LPC_UART0, &c, 1)) {
			uv_streambuffer_push_isr((void*) &u[0]->rx_buffer, &c, 1);
			if (u[0]->callback) {
				u[0]->callback(__uv_get_user_ptr(), UART0);
			}
		}
	}
	// UART_INTEN_TXRDY
	if (Chip_UART_ReadLineStatus(u[0]->uart) & UART_LSR_THRE) {
		if (uv_streambuffer_pop_isr((void*) &u[0]->tx_buffer, &c, 1)) {
			Chip_UART_SendByte(LPC_UART0, c);
		}
		else {
			Chip_UART_IntDisable(LPC_UART0, UART_IER_THREINT);
		}
	}
}
#endif
#if CONFIG_UART1
void UART1_IRQHandler(void) {
}
#endif
#if CONFIG_UART2
void UART2_IRQHandler(void) {
}
#endif






void uv_uart_add_callback(uv_uarts_e uart,
		void (*callback_function)(void* user_ptr, uv_uarts_e uart)) {
	this->callback = callback_function;
}


uv_errors_e _uv_uart_init(uv_uarts_e uart) {
	uv_errors_e ret = ERR_NONE;

#if CONFIG_UART0
	uint32_t baud;
	if (uart == UART0) {
		baud = CONFIG_UART0_BAUDRATE;
		if (uv_streambuffer_init(&this->rx_buffer,
				CONFIG_UART0_RX_BUFFER_SIZE) != ERR_NONE) {
			printf("UART: Not enough RAM for rx buffer\n");
		}
		if (uv_streambuffer_init(&this->tx_buffer,
				CONFIG_UART0_TX_BUFFER_SIZE) != ERR_NONE) {
			printf("UART: Not enough RAM for TX buffer\n");
		}
#if CONFIG_UART0_TX_IO == P0_0
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 0, FUNC4);
#elif CONFIG_UART0_TX_IO == P0_2
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, FUNC1);
#else
#error "CONFIG_UART0_TX_IO should define IO pin"
#endif

#if CONFIG_UART0_RX_IO == P0_1
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 1, FUNC4);
#elif CONFIG_UART0_RX_IO == P0_3
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 3, FUNC1);
#else
#error "CONFIG_UART0_RX_IO should define IO pin"
#endif

		NVIC_SetPriority(UART0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
		NVIC_EnableIRQ(UART0_IRQn);
	}
#endif
#if CONFIG_UART1
	if (uart == UART1) {
#error "UART1 not yet implemented"
	}
#endif
#if CONFIG_UART2
	if (uart == UART2) {
#error "UART1 not yet implemented"
	}
#endif

	Chip_UART_Init(this->uart);
	Chip_UART_SetBaud(this->uart, baud);
	Chip_UART_TXEnable(this->uart);
	Chip_UART_IntEnable(this->uart, UART_IER_RBRINT);

	return ret;
}





uv_errors_e uv_uart_send_char(uv_uarts_e uart, char buffer) {
	uv_errors_e ret = ERR_NONE;

	uv_streambuffer_push(&this->tx_buffer, &buffer, 1, 0);

	return ret;
}



int32_t uv_uart_send(uv_uarts_e uart, char *buffer, uint32_t length) {
	int32_t len = 0;

	if (length) {
		uv_enter_critical();
		len = uv_streambuffer_push((void*) &this->tx_buffer, buffer, length, 0);
		if (Chip_UART_ReadLineStatus(this->uart) & UART_LSR_THRE) {
			char c;
			if (uv_streambuffer_pop((void*) &this->tx_buffer, &c, 1, 0)) {
				Chip_UART_SendByte(this->uart, c);
				Chip_UART_IntEnable(this->uart, UART_IER_THREINT);
			}
		}
		uv_exit_critical();
	}


	return len;
}



uv_errors_e uv_uart_send_str(uv_uarts_e uart, char *buffer) {
	while (*buffer != '\0') {
		uv_uart_send_char(uart, *buffer);

		buffer++;
	}

	return ERR_NONE;
}


int32_t uv_uart_get_tx_free_space(uv_uarts_e uart) {
	return uv_streambuffer_get_free_space((void*) &this->tx_buffer);
}


int32_t uv_uart_get(uv_uarts_e uart, char *dest, uint32_t max_len, int wait_ms) {
	int32_t ret = 0;

	uv_enter_critical();
	ret = uv_streambuffer_pop((void*) &this->rx_buffer, dest, max_len, wait_ms);
	uv_exit_critical();

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


void uv_uart_set_baudrate(uv_uarts_e uart, unsigned int baudrate) {
	Chip_UART_SetBaud(this->uart, baudrate);
}


void uv_uart_clear_rx_buffer(uv_uarts_e uart) {
	uv_streambuffer_clear((void*) &this->rx_buffer);

}



void uv_uart_break_start(uv_uarts_e uart) {
	this->uart->LCR |= UART_LCR_BREAK_EN;
}

void uv_uart_break_stop(uv_uarts_e uart) {
	this->uart->LCR &= ~UART_LCR_BREAK_EN;
}

