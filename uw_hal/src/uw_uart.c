/*
 * uw_uart_controller.c
 *
 *  Created on: Jan 29, 2015
 *      Author: usenius
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#ifdef LPC11C14
#include "LPC11xx.h"
#elif defined(LPC1785)
#include "LPC177x_8x.h"
#endif
#include "uw_terminal.h"
#include "uw_uart.h"
#include <uw_memory.h>
#include "uw_utilities.h"



typedef struct {
	LPC_UART_TypeDef* uart[UART_COUNT];
	void (*callback[UART_COUNT])(void*, uw_uarts_e, char);
} this_st;

/// @brief: Variable which shows if UARTs are initialized or not.
/// This is needed because stdout can use UART and it needs to check
/// whether or not the uart is initialized.
static char uart_init = 0;

static this_st _this;
#define this (&_this)


//UART interrupt handlers
#ifdef LPC11C14
void UART_IRQHandler (void) {
	char received_char = this->uart[UART0]->RBR & 0xFF;
	//call receive callback if it has been registered
	if (this->callback[UART0]) {
		this->callback[UART0](__uw_get_user_ptr(), UART0, received_char);
	}
	// call terminal character process function
	__uw_terminal_process_rx_msg(&received_char, 1, STDOUT_UART);

}
#elif defined(LPC1785)
static void isr(uw_uarts_e uart) {
	char received_char;
	switch (uart) {
	case UART1:
		received_char = ((LPC_UART1_TypeDef*)this->uart[uart])->RBR & 0xFF;
		break;
	case UART4:
		received_char = ((LPC_UART4_TypeDef*)this->uart[uart])->RBR & 0xFF;
		break;
	default:
		received_char = this->uart[uart]->RBR & 0xFF;
		break;
	}
	// call receive callback
	if (this->callback[uart]) {
		this->callback[uart](__uw_get_user_ptr(), uart, received_char);
	}
	// terminal process is valid only for UART0
	if (uart == UART0) {
		__uw_terminal_process_rx_msg(&received_char, 1, STDOUT_UART);
	}
}
void UART0_IRQHandler(void) {
	isr(UART0);
}
void UART1_IRQHandler(void) {
	isr(UART1);
}
void UART2_IRQHandler(void) {
	isr(UART2);
}
void UART3_IRQHandler(void) {
	isr(UART3);
}
void UART4_IRQHandler(void) {
	isr(UART4);
}

#endif


static uw_errors_e check_uart(uw_uarts_e uart) {
	if (uart >= UART_COUNT) {
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_UART);
	}
	return ERR_NONE;
}


uw_errors_e uw_uart_add_callback(uw_uarts_e uart,
		void (*callback_function)(void* user_ptr, uw_uarts_e uart, char chr)) {
	if (check_uart(uart)) return check_uart(uart);
	this->callback[uart] = callback_function;
	return ERR_NONE;
}


#ifdef LPC11C14
uw_errors_e uw_uart_init(uw_uarts_e uart, uint32_t baud_rate) {
	this->callback[UART0] = 0;
	this->uart[UART0] = LPC_UART;

	if (check_uart(uart)) return check_uart(uart);

	SystemCoreClockUpdate();

	//set the baud_rate
	uint32_t uart_clock_div = SystemCoreClock / (baud_rate * 16);
	//uart clock divider is only 8 bits, so discard all values above 255
	if (uart_clock_div > 0xff) {
		uart_clock_div = 0xff;
	}

	// set TX and RX pins
	LPC_IOCON->PIO1_6 |= 0x01;
	LPC_IOCON->PIO1_7 |= 0x01;


	/* Setup the clock and reset UART0 */
	//enable clock for uart
	LPC_SYSCON->SYSAHBCLKCTRL |= (0x01 << 12);
	//set uart clock divider
	LPC_SYSCON->UARTCLKDIV = uart_clock_div;
	//enable uart interrupts
	NVIC_EnableIRQ(UART_IRQn);
	//enable receive data available interrupt
	this->uart[UART0]->IER = 1;

	/* Configure UART0 */
	this->uart[UART0]->LCR = 0x03;
	//baud rate generation: baud_rate = clk / (16 * (U0DLM >> 8 + U0DLL))
	//from that: U0DLM + U0DLL = clk / (16 * baud_rate)
//	this->uart[UART0]->DLL = clk / (16 * baud_rate);
//	this->uart[UART0]->DLM = (clk / (16 * baud_rate)) >> 8;

	//reset UART FIFOs
	this->uart[UART0]->FCR |= (0x01 << 1) | (0x01 << 2);

	/* Enable UART0 */
	this->uart[UART0]->TER |= (0x01 << 7);
	this->uart[UART0]->FCR |= 0x01;
#elif defined(LPC1785)
uw_errors_e uw_uart_init(uw_uarts_e uart, uint32_t baud_rate,
			uw_uart_pins_e tx_pin, uw_uart_pins_e rx_pin) {

	uw_err_pass(check_uart(uart));

	SystemCoreClockUpdate();

	this->callback[uart] = NULL;
	this->uart[UART0] = LPC_UART0;
	this->uart[UART1] = (LPC_UART_TypeDef*) LPC_UART1;
	this->uart[UART2] = LPC_UART2;
	this->uart[UART3] = LPC_UART3;
	this->uart[UART4] = (LPC_UART_TypeDef*) LPC_UART4;

	// set tx and rx pin modes
	*__uw_gpio_get_iocon(tx_pin) &= ~(0b111);
	*__uw_gpio_get_iocon(rx_pin) &= ~(0b111);
	if (uart == UART0) {
		switch (tx_pin) {
		case UART0_TX_PIO0_0: LPC_IOCON->P0_0 |= 0b100; break;
		case UART0_TX_PIO0_2: LPC_IOCON->P0_2 |= 0b001;
		break;
		default: __uw_err_throw(ERR_UNSUPPORTED_PARAM3_VALUE | HAL_MODULE_UART);
		}
		switch (rx_pin) {
		case UART0_RX_PIO0_1: LPC_IOCON->P0_1 |= 0b100; break;
		case UART0_RX_PIO0_3: LPC_IOCON->P0_3 |= 0b001; break;
		default: __uw_err_throw(ERR_UNSUPPORTED_PARAM4_VALUE | HAL_MODULE_UART);
		}
	}
	else if (uart == UART1) {
		switch (tx_pin) {
		case UART1_TX_PIO0_15: LPC_IOCON->P0_15 |= 0b001; break;
		case UART1_TX_PIO2_0: LPC_IOCON->P2_0 |= 0b010; break;
		case UART1_TX_PIO3_16: LPC_IOCON->P3_16 |= 0b011; break;
		default: __uw_err_throw(ERR_UNSUPPORTED_PARAM3_VALUE | HAL_MODULE_UART);
		}
		switch (rx_pin) {
		case UART1_RX_PIO0_16: LPC_IOCON->P0_16 |= 0b001; break;
		case UART1_RX_PIO2_1: LPC_IOCON->P2_1 |= 0b010; break;
		case UART1_RX_PIO3_17: LPC_IOCON->P3_17 |= 0b011; break;
		default: __uw_err_throw(ERR_UNSUPPORTED_PARAM4_VALUE | HAL_MODULE_UART);
		}
	}
	else if (uart == UART2) {
		switch (tx_pin) {
		case UART2_TX_PIO0_10: LPC_IOCON->P0_10 |= 0b001; break;
		case UART2_TX_PIO2_8: LPC_IOCON->P2_8 |= 0b010; break;
		case UART2_TX_PIO4_22: LPC_IOCON->P4_22 |= 0b010; break;
		default: __uw_err_throw(ERR_UNSUPPORTED_PARAM3_VALUE | HAL_MODULE_UART);
		}
		switch (rx_pin) {
		case UART2_RX_PIO0_11: LPC_IOCON->P0_11 |= 0b001; break;
		case UART2_RX_PIO2_9: LPC_IOCON->P2_9 |= 0b010; break;
		case UART2_RX_PIO4_23: LPC_IOCON->P4_23 |= 0b010; break;
		default: __uw_err_throw(ERR_UNSUPPORTED_PARAM4_VALUE | HAL_MODULE_UART);
		}
	}
	else if (uart == UART3) {
		switch (tx_pin) {
		case UART3_TX_PIO0_0: LPC_IOCON->P0_0 |= 0b010; break;
		case UART3_TX_PIO0_2: LPC_IOCON->P0_2 |= 0b010; break;
		case UART3_TX_PIO4_28: LPC_IOCON->P4_28 |= 0b010; break;
		default: __uw_err_throw(ERR_UNSUPPORTED_PARAM3_VALUE | HAL_MODULE_UART);
		}
		switch (rx_pin) {
		case UART3_RX_PIO0_1: LPC_IOCON->P0_1 |= 0b010; break;
		case UART3_RX_PIO0_3: LPC_IOCON->P0_3 |= 0b010; break;
		case UART3_RX_PIO4_29: LPC_IOCON->P4_29 |= 0b010; break;
		default: __uw_err_throw(ERR_UNSUPPORTED_PARAM4_VALUE | HAL_MODULE_UART);
		}
	}
	else if (uart == UART4) {
		switch (tx_pin) {
		case UART4_TX_PIO0_22: LPC_IOCON->P0_22 |= 0b011; break;
		case UART4_TX_PIO1_29: LPC_IOCON->P1_29 |= 0b101; break;
		case UART4_TX_PIO5_4: LPC_IOCON->P5_4 |= 0b100; break;
		default: __uw_err_throw(ERR_UNSUPPORTED_PARAM3_VALUE | HAL_MODULE_UART);
		}
		switch (rx_pin) {
		case UART4_RX_PIO2_9: LPC_IOCON->P2_9 |= 0b011; break;
		case UART4_RX_PIO5_3: LPC_IOCON->P5_3 |= 0b100; break;
		default: __uw_err_throw(ERR_UNSUPPORTED_PARAM4_VALUE | HAL_MODULE_UART);
		}
	}

	// enable clock to uart
	switch (uart) {
	case UART0:
	case UART1:	LPC_SC->PCONP |= (1 << (uart + 3)); break;
	case UART2:
	case UART3: LPC_SC->PCONP |= (1 << (uart + 22)); break;
	case UART4: LPC_SC->PCONP |= (1 << 8); break;
	default:
		break;
	}

	//set baudrate
	uint32_t clock_div = PeripheralClock / (baud_rate * 16);
	// enable access to divisor latch registers and set the register values.
	// divisor latch access is disabled in configurations below
	switch (uart) {
	case UART1:
		((LPC_UART1_TypeDef*)this->uart[uart])->LCR |= (1 << 7);
		((LPC_UART1_TypeDef*)this->uart[uart])->DLL = clock_div & 0xFF;
		((LPC_UART1_TypeDef*)this->uart[uart])->DLM = (clock_div >> 8) & 0xFF;
		((LPC_UART1_TypeDef*)this->uart[uart])->LCR = 0x3;
		((LPC_UART1_TypeDef*)this->uart[uart])->IER = 0x1;
		((LPC_UART1_TypeDef*)this->uart[uart])->FCR = 0x1;
		((LPC_UART1_TypeDef*)this->uart[uart])->TER = (1 << 7);
		break;
	case UART4:
		((LPC_UART4_TypeDef*)this->uart[uart])->LCR |= (1 << 7);
		((LPC_UART4_TypeDef*)this->uart[uart])->DLL = clock_div & 0xFF;
		((LPC_UART4_TypeDef*)this->uart[uart])->DLM = (clock_div >> 8) & 0xFF;
		((LPC_UART4_TypeDef*)this->uart[uart])->LCR = 0x3;
		((LPC_UART4_TypeDef*)this->uart[uart])->IER = 0x1;
		((LPC_UART4_TypeDef*)this->uart[uart])->FCR = 0x1;
		((LPC_UART4_TypeDef*)this->uart[uart])->TER = (1 << 7);
		break;
	default:
		// enable DLAB
		this->uart[uart]->LCR |= (1 << 7);
		// set uart freq
		this->uart[uart]->DLL = clock_div & 0xFF;
		this->uart[uart]->DLM = (clock_div >> 8) & 0xFF;
		// configure uart with 8n1 and disable DLAB bit
		this->uart[uart]->LCR = 0x3;
		// enable receive data interrupt
		this->uart[uart]->IER = 0x1;
		// enable tx and rx FIFOs
		this->uart[uart]->FCR = 0x1;
		// enable transmit
		this->uart[uart]->TER = (1 << 7);
		break;
	}

	// enable interrupts
	if (uart != UART4) {
		NVIC_EnableIRQ(UART0_IRQn + uart);
	}
	else {
		NVIC_EnableIRQ(UART4_IRQn);
	}

#endif

	// this UART is now initialized
	uart_init |= (1 << uart);
	return ERR_NONE;
}


uw_errors_e uw_uart_config(uw_uarts_e uart, uw_uart_configs_e config) {
	if (check_uart(uart)) return check_uart(uart);

#ifdef LPC11C14
	unsigned int c = 0;
	if (GET_MASKED(config, UART_DATA_LENGTH_5)) c &= 0b11;
	else if (GET_MASKED(config, UART_DATA_LENGTH_6)) c |= 1;
	else if (GET_MASKED(config, UART_DATA_LENGTH_7)) c |= 0x2;
	else if (GET_MASKED(config, UART_DATA_LENGTH_8)) c |= 0x3;
	if (GET_MASKED(config, UART_STOP_BITS_2)) c |= (1 << 2);
	if (GET_MASKED(config, UART_PARITY_ODD)) c |= (1 << 3);
	else if (GET_MASKED(config, UART_PARITY_EVEN)) c |= (0b11 << 3);

	this->uart[UART0]->LCR = c;
	return ERR_NONE;
#elif defined(LPC1785)
	unsigned int c = 0;
	if (GET_MASKED(config, UART_DATA_LENGTH_5)) c &= 0b11;
	else if (GET_MASKED(config, UART_DATA_LENGTH_6)) c |= 1;
	else if (GET_MASKED(config, UART_DATA_LENGTH_7)) c |= 0x2;
	else if (GET_MASKED(config, UART_DATA_LENGTH_8)) c |= 0x3;
	if (GET_MASKED(config, UART_STOP_BITS_2)) c |= (1 << 2);
	if (GET_MASKED(config, UART_PARITY_ODD)) c |= (1 << 3);
	else if (GET_MASKED(config, UART_PARITY_EVEN)) c |= (0b11 << 3);

	switch (uart) {
	case UART1:
		((LPC_UART1_TypeDef*)this->uart[uart])->LCR = c;
		break;
	case UART4:
		((LPC_UART4_TypeDef*)this->uart[uart])->LCR = c;
		break;
	default:
		this->uart[uart]->LCR = c;
		break;
	}
	return ERR_NONE;
#endif
}



uw_errors_e uw_uart_send_char(uw_uarts_e uart, char buffer) {
	if (check_uart(uart)) return check_uart(uart);

	if (!(uart_init & (1 << uart))) {
		__uw_err_throw(ERR_NOT_INITIALIZED | HAL_MODULE_UART);
	}

#ifdef LPC11C14
	/* Wait until we're ready to send */
	while (!(this->uart[UART0]->LSR & (1 << 6))) ;
	//send data
	this->uart[UART0]->THR = buffer;
#elif defined(LPC1785)
	/* Wait until we're ready to send */
	switch (uart) {
	case UART1:
		while (!(((LPC_UART1_TypeDef*)this->uart[uart])->LSR & (1 << 6))) ;
		((LPC_UART1_TypeDef*)this->uart[uart])->THR = buffer;
		break;
	case UART4:
		while (!(((LPC_UART4_TypeDef*)this->uart[uart])->LSR & (1 << 6))) ;
		((LPC_UART4_TypeDef*)this->uart[uart])->THR = buffer;
		break;
	default:
		while (!(this->uart[uart]->LSR & (1 << 6))) ;
			this->uart[uart]->THR = buffer;
		break;
	}
#endif

	return ERR_NONE;
}



uw_errors_e uw_uart_send(uw_uarts_e uart, char *buffer, uint32_t length) {
	if (check_uart(uart)) return check_uart(uart);

	while (length != 0) {
		uw_uart_send_char(uart, *buffer);
		buffer++;
		length--;
	}
	return ERR_NONE;
}



uw_errors_e uw_uart_send_str(uw_uarts_e uart, char *buffer) {
	if (check_uart(uart)) return check_uart(uart);
	while (*buffer != '\0') {
		uw_uart_send_char(uart, *buffer);

		buffer++;
	}

	return ERR_NONE;
}

bool uw_uart_is_initialized(uw_uarts_e uart) {
	return (uart_init & (1 << uart));
}


