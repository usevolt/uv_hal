/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "uv_uart.h"


#if CONFIG_UART

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#include "uv_gpio.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#elif CONFIG_TARGET_LPC1549
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


// errors if UARTs which do not exist have been enabled
#if (CONFIG_TARGET_LPC11C14)
#if (!CONFIG_UART0)
#warning "UART0 not enabled. Since UART0 is the only UART in LPC11Cxx, it\
is recommended to be enabled."
#endif
#if CONFIG_UART1
#error "UART1 not available on LPC11Cxx. Please disable UART1 from uv_hal_config."
#endif
#if CONFIG_UART2
#error "UART2 not available on LPC11Cxx. Please disable UART2 from uv_hal_config."
#endif
#if CONFIG_UART3
#error "UART3 not available on LPC11Cxx. Please disable UART3 from uv_hal_config."
#endif
#if CONFIG_UART4
#error "UART4 not available on LPC11Cxx. Please disable UART4 from uv_hal_config."
#endif
#endif


// Receive buffers for all enabled uart's
#if CONFIG_UART0
static char uart0_rxbuffer[CONFIG_UART0_RX_BUFFER_SIZE];
#endif
#if CONFIG_UART1
static char uart1_rxbuffer[CONFIG_UART1_RX_BUFFER_SIZE];
#endif
#if CONFIG_UART2
static char uart2_rxbuffer[CONFIG_UART2_RX_BUFFER_SIZE];
#endif
#if CONFIG_UART3
static char uart3_rxbuffer[CONFIG_UART3_RX_BUFFER_SIZE];
#endif
#if CONFIG_UART4
static char uart4_rxbuffer[CONFIG_UART4_RX_BUFFER_SIZE];
#endif




typedef struct {
#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1785
	LPC_UART_TypeDef* uart[UART_COUNT];
#endif
	void (*callback[UART_COUNT])(void*, uv_uarts_e);
	uv_ring_buffer_st buffer[UART_COUNT];
} this_st;




static this_st _this;

#define this (&_this)


//UART interrupt handlers
#if (CONFIG_TARGET_LPC11C14 && CONFIG_UART0)
void UART_IRQHandler (void) {

	char received_char = this->uart[UART0]->RBR & 0xFF;
	uv_err_check(uv_ring_buffer_push(&this->buffer[UART0], &received_char)) {
		if (uv_get_error() == ERR_BUFFER_OVERFLOW) {
			uv_log_error(__uv_error);
		}
	}
	// callback
	if (this->callback[UART0]) {
		this->callback[UART0](__uv_get_user_ptr(), UART0);
	}
}
#elif CONFIG_TARGET_LPC1785
static void isr(uv_uarts_e uart) {
	char received_char;
#if CONFIG_UART1
	if (uart == UART1) received_char = ((LPC_UART1_TypeDef*)this->uart[uart])->RBR & 0xFF;
#endif
#if CONFIG_UART4
	if (uart == UART4) received_char = ((LPC_UART4_TypeDef*)this->uart[uart])->RBR & 0xFF;
#endif
#if (CONFIG_UART1 || CONFIG_UART4)
	else
#endif
	received_char = this->uart[uart]->RBR & 0xFF;
	uv_ring_buffer_push(&this->buffer[uart], &received_char);
	// call callback
	if (this->callback[uart]) {
		this->callback[uart](__uv_get_user_ptr(), uart);
	}
}
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
#if CONFIG_UART3
void UART3_IRQHandler(void) {
	isr(UART3);
}
#endif
#if CONFIG_UART4
void UART4_IRQHandler(void) {
	isr(UART4);
}
#endif
#elif CONFIG_TARGET_LPC1549
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
		uv_errors_e e = uv_ring_buffer_push(&this->buffer[i], &c);
		if (e) {
			uv_log_error(e);
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
#if CONFIG_TARGET_LPC1549
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


#if (CONFIG_TARGET_LPC11C14 && CONFIG_UART0)
uv_errors_e _uv_uart_init(uv_uarts_e uart) {
	this->callback[UART0] = 0;
	this->uart[UART0] = LPC_UART;
	uv_ring_buffer_init(this->buffer, uart0_rxbuffer, CONFIG_UART0_RX_BUFFER_SIZE, sizeof(char));

	SystemCoreClockUpdate();

	//set the baud_rate
	uint32_t uart_clock_div = SystemCoreClock / (CONFIG_UART0_BAUDRATE * 16);
//	//uart clock divider is only 8 bits, so discard all values above 255
//	if (uart_clock_div > 0xff) {
//		uart_clock_div = 0xff;
//	}

	// set TX and RX pins
	LPC_IOCON->PIO1_6 = 0b110001;
	LPC_IOCON->PIO1_7 = 0b110001;


	/* Setup the clock and reset UART0 */
	//enable clock for uart
	LPC_SYSCON->SYSAHBCLKCTRL |= (0x01 << 12);
//	//set uart clock divider
//	LPC_SYSCON->UARTCLKDIV = uart_clock_div;
	//enable uart interrupts
	NVIC_EnableIRQ(UART_IRQn);
	//enable receive data available interrupt
	this->uart[UART0]->IER = 1;

	/* Configure UART0 */
#if CONFIG_UART0_DATA_5_BYTES
	uint32_t config = 0;
#elif CONFIG_UART0_DATA_6_BYTES
	uint32_t config = 1;
#elif CONFIG_UART0_DATA_7_BYTES
	uint32_t config = 2;
#elif CONFIG_UART0_DATA_8_BYTES
	uint32_t config = 3;
#else
#error "No data length specified for UART0"
#endif
#if CONFIG_UART0_PARITY_NONE
#elif CONFIG_UART0_PARITY_EVEN
	config |= (0b11 << 3);
#elif CONFIG_UART0_PARITY_ODD
	config |= (1 << 3);
#else
#error "No parity specified for UART0"
#endif
#if CONFIG_UART0_STOP_BITS_1
#elif CONFIG_UART0_STOP_BITS_2
	config |= (1 << 2);
#else
#error "No stop bit count specified for UART0"
#endif
	//baud rate generation: baud_rate = clk / (16 * (U0DLM >> 8 + U0DLL))
	//from that: U0DLM + U0DLL = clk / (16 * baud_rate)


	// enable DLAB
	this->uart[UART0]->LCR |= (1 << 7);
	this->uart[uart]->DLL = uart_clock_div & 0xFF;
	this->uart[uart]->DLM = (uart_clock_div >> 8) & 0xFF;

	// disable DLAB and set configs
	this->uart[UART0]->LCR = config;

	//reset UART FIFOs
	this->uart[UART0]->FCR |= (0x01 << 1) | (0x01 << 2);

	/* Enable UART0 */
	this->uart[UART0]->TER |= (0x01 << 7);
	this->uart[UART0]->FCR |= 0x01;

	return uv_err(ERR_NONE);
}

#elif CONFIG_TARGET_LPC1785
uv_errors_e _uv_uart_init(uv_uarts_e uart) {
	uv_errors_e ret = ERR_NONE;

	SystemCoreClockUpdate();

	this->callback[uart] = NULL;

	uint32_t clock_div;
	uint32_t lcr = 0;
#if CONFIG_UART0
	if (uart == UART0) {
		uv_ring_buffer_init(&this->buffer[uart], uart0_rxbuffer, CONFIG_UART0_RX_BUFFER_SIZE, sizeof(char));
#if CONFIG_UART0_DATA_5_BYTES
#elif CONFIG_UART0_DATA_6_BYTES
			lcr = 1;
#elif CONFIG_UART0_DATA_7_BYTES
			lcr = 2;
#elif CONFIG_UART0_DATA_8_BYTES
			lcr = 3;
#else
#error "No data length set for UART0"
#endif
#if CONFIG_UART0_PARITY_NONE
#elif CONFIG_UART0_PARITY_EVEN
			config |= (0b11 << 3);
#elif CONFIG_UART0_PARITY_ODD
			config |= (0b1 << 3);
#else
#error "No parity bits set for UART0"
#endif
	}
#endif
#if CONFIG_UART1
		if (uart == UART1) {
			uv_ring_buffer_init(&this->buffer[uart], uart1_rxbuffer, CONFIG_UART1_RX_BUFFER_SIZE);
#if CONFIG_UART1_DATA_5_BYTES
#elif CONFIG_UART1_DATA_6_BYTES
			lcr = 1;
#elif CONFIG_UART1_DATA_7_BYTES
			lcr = 2;
#elif CONFIG_UART1_DATA_8_BYTES
			lcr = 3;
#else
#error "No data length set for UART1"
#endif
#if CONFIG_UART1_PARITY_NONE
#elif CONFIG_UART1_PARITY_EVEN
			config |= (0b11 << 3);
#elif CONFIG_UART1_PARITY_ODD
			config |= (0b1 << 3);
#else
#error "No parity bits set for UART1"
#endif
		}
#endif
#if CONFIG_UART2
		if (uart == UART2) {
			uv_ring_buffer_init(&this->buffer[uart], uart2_rxbuffer, CONFIG_UART2_RX_BUFFER_SIZE);
#if CONFIG_UART2_DATA_5_BYTES
#elif CONFIG_UART2_DATA_6_BYTES
			lcr = 1;
#elif CONFIG_UART2_DATA_7_BYTES
			lcr = 2;
#elif CONFIG_UART2_DATA_8_BYTES
			lcr = 3;
#else
#error "No data length set for UART2"
#endif
#if CONFIG_UART2_PARITY_NONE
#elif CONFIG_UART2_PARITY_EVEN
			config |= (0b11 << 3);
#elif CONFIG_UART2_PARITY_ODD
			config |= (0b1 << 3);
#else
#error "No parity bits set for UART2"
#endif
		}
#endif
#if CONFIG_UART3
		if (uart == UART3) {
			uv_ring_buffer_init(&this->buffer[uart], uart3_rxbuffer, CONFIG_UART3_RX_BUFFER_SIZE);
#if CONFIG_UART3_DATA_5_BYTES
#elif CONFIG_UART3_DATA_6_BYTES
			lcr = 1;
#elif CONFIG_UART3_DATA_7_BYTES
			lcr = 2;
#elif CONFIG_UART3_DATA_8_BYTES
			lcr = 3;
#else
#error "No data length set for UART3"
#endif
#if CONFIG_UART3_PARITY_NONE
#elif CONFIG_UART3_PARITY_EVEN
			config |= (0b11 << 3);
#elif CONFIG_UART3_PARITY_ODD
			config |= (0b1 << 3);
#else
#error "No parity bits set for UART3"
#endif
		}
#endif
#if CONFIG_UART4
		if (uart == UART4) {
			uv_ring_buffer_init(&this->buffer[uart], uart4_rxbuffer, CONFIG_UART4_RX_BUFFER_SIZE);
#if CONFIG_UART4_DATA_5_BYTES
#elif CONFIG_UART4_DATA_6_BYTES
			lcr = 1;
#elif CONFIG_UART4_DATA_7_BYTES
			lcr = 2;
#elif CONFIG_UART4_DATA_8_BYTES
			lcr = 3;
#else
#error "No data length set for UART4"
#endif
#if CONFIG_UART4_PARITY_NONE
#elif CONFIG_UART4_PARITY_EVEN
			config |= (0b11 << 3);
#elif CONFIG_UART4_PARITY_ODD
			config |= (0b1 << 3);
#else
#error "No parity bits set for UART4"
#endif
		}
#endif

	switch (uart) {
#if CONFIG_UART0
	case UART0:
		this->uart[UART0] = LPC_UART0;
#if CONFIG_UART0_TX_PIO0_0
		LPC_IOCON->P0_0 = 0b110100;
#elif CONFIG_UART0_TX_PIO0_2
		LPC_IOCON->P0_2 = 0b110001;
#else
#error "No TX pins specified for UART0"
#endif
#if CONFIG_UART0_RX_PIO0_1
		LPC_IOCON->P0_1 = 0b110100;
#elif CONFIG_UART0_RX_PIO0_3
		LPC_IOCON->P0_3 = 0b110001;
#else
#error "No RX pins specified for UART0"
#endif
		// enabled clock
		LPC_SC->PCONP |= (1 << (uart + 3));
		// calculate baudrate
		clock_div = PeripheralClock / (CONFIG_UART0_BAUDRATE * 16);
		// enable DLAB
		this->uart[uart]->LCR |= (1 << 7);
		// set uart freq
		this->uart[uart]->DLL = clock_div & 0xFF;
		this->uart[uart]->DLM = (clock_div >> 8) & 0xFF;
		// configure uart mode
		this->uart[uart]->LCR = lcr;
		// enable receive data interrupt
		this->uart[uart]->IER = 0x1;
		// enable tx and rx FIFOs
		this->uart[uart]->FCR = 0x1;
		// enable transmit
		this->uart[uart]->TER = (1 << 7);
		break;
#endif
#if CONFIG_UART1
	case UART1:
		this->uart[UART1] = (LPC_UART_TypeDef*) LPC_UART1;
#if CONFIG_UART1_TX_PIO0_15
		LPC_IOCON->P0_15 = 0b110001;
#elif CONFIG_UART1_TX_PIO2_0
		LPC_IOCON->P2_0 = 0b110010;
#elif CONFIG_UART1_TX_PIO3_16
		LPC_IOCON->P3_16 = 0b110011;
#else
#error "No TX pins specified for UART1"
#endif
#if CONFIG_UART1_RX_PIO0_16
		LPC_IOCON->P0_16 = 0b110001;
#elif CONFIG_UART1_RX_PIO2_1
		LPC_IOCON->P2_1 = 0b110010;
#elif CONFIG_UART1_RX_PIO3_17
		LPC_IOCON->P3_17 = 0b110011;
#else
#error "No RX pins specified for UART1"
#endif
		// enabled clock
		LPC_SC->PCONP |= (1 << (uart + 3));
		// calculate baudrate
		clock_div = PeripheralClock / (CONFIG_UART1_BAUDRATE * 16);
		((LPC_UART1_TypeDef*)this->uart[uart])->LCR |= (1 << 7);
		((LPC_UART1_TypeDef*)this->uart[uart])->DLL = clock_div & 0xFF;
		((LPC_UART1_TypeDef*)this->uart[uart])->DLM = (clock_div >> 8) & 0xFF;
		((LPC_UART1_TypeDef*)this->uart[uart])->LCR = lcr;
		((LPC_UART1_TypeDef*)this->uart[uart])->IER = 0x1;
		((LPC_UART1_TypeDef*)this->uart[uart])->FCR = 0x1;
		((LPC_UART1_TypeDef*)this->uart[uart])->TER = (1 << 7);
		break;
#endif
#if CONFIG_UART2
	case UART2:
		this->uart[UART2] = LPC_UART2;
#if CONFIG_UART2_TX_PIO0_10
		LPC_IOCON->P0_10 = 0b110001;
#elif CONFIG_UART2_TX_PIO2_8
		LPC_IOCON->P2_8 = 0b110010;
#elif CONFIG_UART2_TX_PIO4_22
		LPC_IOCON->P4_22 = 0b110010;
#else
#error "No TX pins specified for UART2"
#endif
#if CONFIG_UART2_RX_PIO0_11
		LPC_IOCON->P0_11 = 0b110001;
#elif CONFIG_UART2_RX_PIO2_9
		LPC_IOCON->P2_9 = 0b110010;
#elif CONFIG_UART2_RX_PIO4_23
		LPC_IOCON->P4_23 = 0b110010;
#else
#error "No RX pins specified for UART2"
#endif
		// enable clock to uart
		LPC_SC->PCONP |= (1 << (uart + 22));
		// calculate baudrate
		clock_div = PeripheralClock / (CONFIG_UART2_BAUDRATE * 16);
		// enable DLAB
		this->uart[uart]->LCR |= (1 << 7);
		// set uart freq
		this->uart[uart]->DLL = clock_div & 0xFF;
		this->uart[uart]->DLM = (clock_div >> 8) & 0xFF;
		// configure uart with 8n1 and disable DLAB bit
		this->uart[uart]->LCR = lcr;
		// enable receive data interrupt
		this->uart[uart]->IER = 0x1;
		// enable tx and rx FIFOs
		this->uart[uart]->FCR = 0x1;
		// enable transmit
		this->uart[uart]->TER = (1 << 7);
		break;
#endif
#if CONFIG_UART3
	case UART3:
		this->uart[UART3] = LPC_UART3;
#if CONFIG_UART3_TX_PIO0_0
		LPC_IOCON->P0_0 = 0b110010;
#elif CONFIG_UART3_TX_PIO0_2
		LPC_IOCON->P0_2 = 0b110010;
#elif CONFIG_UART3_TX_PIO4_28
		LPC_IOCON->P4_28 = 0b110010;
#else
#error "No TX pins specified for UART3"
#endif
#if CONFIG_UART3_RX_PIO0_1
		LPC_IOCON->P0_1 = 0b110010;
#elif CONFIG_UART3_RX_PIO0_3
		LPC_IOCON->P0_3 = 0b110010;
#elif CONFIG_UART3_RX_PIO4_29
		LPC_IOCON->P4_29 = 0b110010;
#else
#error "No RX pins specified for UART3"
#endif
		// enable clock to uart
		LPC_SC->PCONP |= (1 << (uart + 22));
		// calculate baudrate
		clock_div = PeripheralClock / (CONFIG_UART3_BAUDRATE * 16);
		// enable DLAB
		this->uart[uart]->LCR |= (1 << 7);
		// set uart freq
		this->uart[uart]->DLL = clock_div & 0xFF;
		this->uart[uart]->DLM = (clock_div >> 8) & 0xFF;
		// configure uart with 8n1 and disable DLAB bit
		this->uart[uart]->LCR = lcr;
		// enable receive data interrupt
		this->uart[uart]->IER = 0x1;
		// enable tx and rx FIFOs
		this->uart[uart]->FCR = 0x1;
		// enable transmit
		this->uart[uart]->TER = (1 << 7);
		break;
#endif
#if CONFIG_UART4
	case UART4:
		this->uart[UART4] = (LPC_UART_TypeDef*) LPC_UART4;
#if CONFIG_UART4_TX_PIO0_22
		LPC_IOCON->P0_22 = 0b110011;
#elif CONFIG_UART4_TX_PIO1_29
		LPC_IOCON->P1_29 = 0b110101;
#elif CONFIG_UART4_TX_PIO5_4
		LPC_IOCON->P5_4 = 0b110100;
#else
#error "No TX pins specified for UART4"
#endif
#if CONFIG_UART4_RX_PIO2_9
		LPC_IOCON->P2_9 = 0b110011;
#elif CONFIG_UART4_RX_PIO5_3
		LPC_IOCON->P5_3 = 0b110;
#else
#error "No RX pins specified for UART4"
#endif
		// enable clock to UART
		LPC_SC->PCONP |= (1 << 8);
		// calculate baudrate
		clock_div = PeripheralClock / (CONFIG_UART4_BAUDRATE * 16);
		((LPC_UART4_TypeDef*)this->uart[uart])->LCR |= (1 << 7);
		((LPC_UART4_TypeDef*)this->uart[uart])->DLL = clock_div & 0xFF;
		((LPC_UART4_TypeDef*)this->uart[uart])->DLM = (clock_div >> 8) & 0xFF;
		((LPC_UART4_TypeDef*)this->uart[uart])->LCR = lcr;
		((LPC_UART4_TypeDef*)this->uart[uart])->IER = 0x1;
		((LPC_UART4_TypeDef*)this->uart[uart])->FCR = 0x1;
		((LPC_UART4_TypeDef*)this->uart[uart])->TER = (1 << 7);
		break;
#endif
	default:
		// this should not happen if hal_config was correctly configured
		ret = ERR_INCORRECT_HAL_CONFIG;
		break;
	}


	// enable interrupts
#if CONFIG_UART4
	if (uart != UART4) {
#endif
	NVIC_EnableIRQ(UART0_IRQn + uart);
#if CONFIG_UART4
	}
	else {
		NVIC_EnableIRQ(UART4_IRQn);
	}
#endif


	return ret;
}
#elif CONFIG_TARGET_LPC1549
uv_errors_e _uv_uart_init(uv_uarts_e uart) {
	uv_errors_e ret = ERR_NONE;

	Chip_Clock_SetUARTBaseClockRate(Chip_Clock_GetMainClockRate(), false);

	#if CONFIG_UART0
	uint32_t baud;
	if (uart == UART0) {
		baud = CONFIG_UART0_BAUDRATE;
		uv_ring_buffer_init(&this->buffer[0], uart0_rxbuffer, CONFIG_UART0_RX_BUFFER_SIZE, sizeof(char));
		Chip_IOCON_PinMuxSet(LPC_IOCON, uv_gpio_port(CONFIG_UART0_TX_PIN),
				uv_gpio_pin(CONFIG_UART0_TX_PIN), (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_IOCON_PinMuxSet(LPC_IOCON, uv_gpio_port(CONFIG_UART0_RX_PIN),
				uv_gpio_pin(CONFIG_UART0_RX_PIN), (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));

		Chip_SWM_MovablePortPinAssign(SWM_UART0_TXD_O,
				uv_gpio_port(CONFIG_UART0_TX_PIN), uv_gpio_pin(CONFIG_UART0_TX_PIN));
		Chip_SWM_MovablePortPinAssign(SWM_UART0_RXD_I,
				uv_gpio_port(CONFIG_UART0_RX_PIN), uv_gpio_pin(CONFIG_UART0_RX_PIN));
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

#if (CONFIG_TARGET_LPC11C14 && CONFIG_UART0)
	/* Wait until we're ready to send */
	while (!(this->uart[UART0]->LSR & (1 << 6))) {
		uv_rtos_task_delay(1);
	}
	//send data
	this->uart[UART0]->THR = buffer;
#elif CONFIG_TARGET_LPC1785
	/* Wait until we're ready to send */
	switch (uart) {
#if CONFIG_UART1
	case UART1:
		while (!(((LPC_UART1_TypeDef*)this->uart[uart])->LSR & (1 << 6))) ;
		((LPC_UART1_TypeDef*)this->uart[uart])->THR = buffer;
		break;
#endif
#if CONFIG_UART4
	case UART4:
		while (!(((LPC_UART4_TypeDef*)this->uart[uart])->LSR & (1 << 6))) ;
		((LPC_UART4_TypeDef*)this->uart[uart])->THR = buffer;
		break;
#endif
	default:
		while (!(this->uart[uart]->LSR & (1 << 6))) ;
			this->uart[uart]->THR = buffer;
		break;
	}
#elif CONFIG_TARGET_LPC1549
	Chip_UART_SendBlocking((void *) uart, &buffer, 1);
#endif

	return ret;
}



uv_errors_e uv_uart_send(uv_uarts_e uart, char *buffer, uint32_t length) {

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


uv_errors_e uv_uart_get_char(uv_uarts_e uart, char *dest) {
	uv_errors_e ret = ERR_NONE;

	uv_disable_int();
	uint32_t i = uart;
#if CONFIG_TARGET_LPC1549
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
	ret = uv_ring_buffer_pop(&this->buffer[i], dest);
	uv_enable_int();
	return ret;
}




#endif
