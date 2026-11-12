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
	uv_gpios_e cts;
	// false makes the transmitter ignore the CTS input. A peer that only
	// starts to drive its RTS output once flow control is negotiated
	// (ESP-AT before AT+UART_CUR) leaves the line floating until then.
	bool cts_enabled;
} uart_st;


// RTS is raised ("stop sending") when fewer than this many bytes are free in
// the RX ring. Everything that sits in the 16-byte hardware RX FIFO when RTS
// goes up, plus the byte or two the peer still has in flight, has to fit.
#define UART_RTS_HIGH_WATER			32

// Bytes written to the 16-byte TX FIFO per THRE interrupt. CTS is checked
// before each, but bytes already in the FIFO go out even if the peer raises
// its RTS meanwhile. ESP-AT raises RTS at 120 bytes in its 128-byte RX FIFO,
// so up to 4 queued bytes plus the one in the shift register still fit.
#define UART_TX_BURST				4

// The RX FIFO interrupts at 4 bytes (or after a 3.5 - 4.5 character idle
// time with fewer), leaving 12 bytes of headroom for interrupt latency.
#define UART_FCR_CONFIG				(UART_FCR_FIFO_EN | UART_FCR_TRG_LEV1)

#if CONFIG_UART0
static uart_st uart0 = {
		.uart = LPC_UART0,
		.callback = NULL
#ifdef CONFIG_UART0_CTS_IO
		,
		.cts = CONFIG_UART0_CTS_IO,
		.cts_enabled = true
#endif
};
#endif
#if CONFIG_UART1
static uart_st uart1 = {
		.uart = LPC_UART1,
		.callback = NULL
#ifdef CONFIG_UART1_CTS_IO
		,
		.cts = CONFIG_UART1_CTS_IO,
		.cts_enabled = true
#endif
};
#endif
#if CONFIG_UART2
static uart_st uart2 = {
		.uart = LPC_UART2,
		.callback = NULL
#ifdef CONFIG_UART2_CTS_IO
		,
		.cts = CONFIG_UART2_CTS_IO,
		.cts_enabled = true
#endif
};
#endif
#if CONFIG_UART3
static uart_st uart3 = {
		.uart = LPC_UART3,
		.callback = NULL
#ifdef CONFIG_UART3_CTS_IO
		,
		.cts = CONFIG_UART3_CTS_IO,
		.cts_enabled = true
#endif
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


static inline bool cts(uart_st *uart) {
	bool ret = true;
	if (uart->cts && uart->cts_enabled) {
		ret = !uv_gpio_get(uart->cts);
	}
	return ret;
}


/// @brief: Drives RTS from the RX ring fill level. Called with the UART
/// interrupt masked (ISR or critical section) so that the level read and
/// the pin written cannot be overtaken by a byte the ISR pushes in between.
static inline void update_rts(uv_uarts_e uart) {
#if CONFIG_UART0 && (CONFIG_UART0_RTS_IO != 0)
	if (uart == UART0) {
		uv_gpio_set(CONFIG_UART0_RTS_IO,
					uv_streambuffer_get_free_space(&u[0]->rx_buffer) <
					UART_RTS_HIGH_WATER);
	}
#endif
}


//UART interrupt handlers
#if CONFIG_UART0
void UART0_IRQHandler(void) {
	char c;
	bool received = false;
	// Drain the whole RX FIFO: it interrupts at its trigger level or on a
	// character timeout, and both are cleared by reading it empty.
	while (Chip_UART_ReadLineStatus(LPC_UART0) & UART_LSR_RDR) {
		c = (char) Chip_UART_ReadByte(LPC_UART0);
		uv_streambuffer_push_isr((void*) &u[0]->rx_buffer, &c, 1);
		received = true;
	}
	if (received) {
		update_rts(UART0);
		if (u[0]->callback) {
			u[0]->callback(__uv_get_user_ptr(), UART0);
		}
	}
	else {
	}
	// UART_INTEN_TXRDY
	if (Chip_UART_ReadLineStatus(LPC_UART0) & UART_LSR_THRE) {
		uint32_t i;
		for (i = 0; i < UART_TX_BURST; i++) {
			if (cts(u[0]) &&
					uv_streambuffer_pop_isr((void*) &u[0]->tx_buffer, &c, 1)) {
				Chip_UART_SendByte(LPC_UART0, c);
			}
			else {
				break;
			}
		}
		if (i == 0) {
			// nothing to send, or CTS holds it back: the hal step restarts
			// the transmission
			Chip_UART_IntDisable(LPC_UART0, UART_IER_THREINT);
		}
		else {
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


/// @brief: Sets the divisor latch and the fractional divider for the rate
/// closest to *baudrate*:
/// baud = PCLK / (16 * DL * (1 + DIVADDVAL / MULVAL)).
/// Replaces Chip_UART_SetBaudFDR, which writes MULVAL 0 (not allowed) when
/// the rate divides PCLK evenly, and misses closer settings otherwise
/// (115200 came out 1.7 % fast at 60 MHz PCLK).
static void set_baud(LPC_USART_T *uart, uint32_t baudrate) {
	uint32_t pclk = Chip_Clock_GetPeripheralClockRate();
	uint32_t best_err = UINT32_MAX;
	uint32_t best_dl = 1;
	uint32_t best_mul = 1;
	uint32_t best_div = 0;
	uint32_t mul;
	for (mul = 1; mul <= 15; mul++) {
		uint32_t div;
		for (div = 0; div < mul; div++) {
			// DL = PCLK * MUL / (16 * baud * (MUL + DIV)), rounded
			uint64_t den = (uint64_t) 16 * baudrate * (mul + div);
			uint32_t dl = (uint32_t) (((uint64_t) pclk * mul + den / 2) / den);
			// with the fractional divider on, DL has to be at least 3
			if ((dl >= ((div == 0) ? 1U : 3U)) && (dl <= 0xFFFF)) {
				uint32_t actual = (uint32_t) (((uint64_t) pclk * mul) /
						((uint64_t) 16 * dl * (mul + div)));
				uint32_t err = (actual > baudrate) ?
						(actual - baudrate) : (baudrate - actual);
				if (err < best_err) {
					best_err = err;
					best_dl = dl;
					best_mul = mul;
					best_div = div;
				}
				else {
				}
			}
			else {
			}
		}
	}
	Chip_UART_EnableDivisorAccess(uart);
	Chip_UART_SetDivisorLatches(uart, UART_LOAD_DLL(best_dl),
								UART_LOAD_DLM(best_dl));
	Chip_UART_DisableDivisorAccess(uart);
	uart->FDR = (UART_FDR_MULVAL(best_mul) | UART_FDR_DIVADDVAL(best_div));
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
#ifdef CONFIG_UART0_CTS_IO
		uv_gpio_init_input(CONFIG_UART0_CTS_IO, PULL_DOWN_ENABLED);
#endif

		NVIC_SetPriority(UART0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
		NVIC_EnableIRQ(UART0_IRQn);
#endif
#if CONFIG_UART0_RTS_IO != 0
		// initialize RTS (request to send) as false, meaning
		// there is space in rx buffer to receive data
		uv_gpio_init_output(CONFIG_UART0_RTS_IO, false);
#endif
	}




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
	Chip_UART_SetupFIFOS(this->uart, UART_FCR_CONFIG |
						 UART_FCR_RX_RS | UART_FCR_TX_RS);
	set_baud(this->uart, baud);
	Chip_UART_TXEnable(this->uart);
	Chip_UART_IntEnable(this->uart, UART_IER_RBRINT);

	return ret;
}





uv_errors_e uv_uart_send_char(uv_uarts_e uart, char buffer) {
	uv_errors_e ret = ERR_NONE;

	uv_streambuffer_push(&this->tx_buffer, &buffer, 1, 0);

	return ret;
}


static void send_next_byte(uv_uarts_e uart) {
	if (Chip_UART_ReadLineStatus(this->uart) & UART_LSR_THRE) {
		char c;
		if (cts(this) &&
				uv_streambuffer_pop((void*) &this->tx_buffer, &c, 1, 0)) {
			Chip_UART_SendByte(this->uart, c);
			Chip_UART_IntEnable(this->uart, UART_IER_THREINT);
		}
	}
}


int32_t uv_uart_send(uv_uarts_e uart, char *buffer, uint32_t length) {
	int32_t len = 0;

	if (length) {
		uv_enter_critical();
		len = uv_streambuffer_push((void*) &this->tx_buffer, buffer, length, 0);
		send_next_byte(uart);
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

	// Not inside a critical section: a blocking receive must be able to
	// block. The ring has a single reader (this) and a single writer (the
	// ISR), which FreeRTOS stream buffers support without extra locking.
	ret = uv_streambuffer_pop((void*) &this->rx_buffer, dest, max_len, wait_ms);
	uv_enter_critical();
	update_rts(uart);
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
	uv_enter_critical();
	// let the byte being shifted out finish at the old rate
	while (!(Chip_UART_ReadLineStatus(this->uart) & UART_LSR_TEMT)) {
	}
	set_baud(this->uart, baudrate);
	uv_exit_critical();
}


void uv_uart_clear_rx_buffer(uv_uarts_e uart) {
	uv_streambuffer_clear((void*) &this->rx_buffer);
	uv_enter_critical();
	update_rts(uart);
	uv_exit_critical();
}


void uv_uart_set_cts_enabled(uv_uarts_e uart, bool value) {
	uv_enter_critical();
	this->cts_enabled = value;
	// restart a transmission that CTS had stalled, rather than waiting for
	// the next hal step
	send_next_byte(uart);
	uv_exit_critical();
}



void uv_uart_break_start(uv_uarts_e uart) {
	this->uart->LCR |= UART_LCR_BREAK_EN;
}

void uv_uart_break_stop(uv_uarts_e uart) {
	this->uart->LCR &= ~UART_LCR_BREAK_EN;
}


void _uv_uart_hal_step(uint16_t step_ms) {
	// if transmission is not ongoing, try next character
	uv_enter_critical();
	send_next_byte(UART0);
	uv_exit_critical();
}
