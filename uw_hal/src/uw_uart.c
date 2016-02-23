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

#endif

#include "uw_terminal.h"
#include "uw_uart.h"
#include <uw_memory.h>
#include "uw_utilities.h"



typedef struct {
#ifdef LPC11C14
	LPC_UART_TypeDef* uart[UART_COUNT];
#elif defined(LPC1785)
#endif
	void (*callback[UART_COUNT])(void*, char);
} this_st;

static this_st _this;
#define this (&_this)


//UART interrupt handlers
#ifdef LPC11C14
void UART_IRQHandler (void) {
	char received_char = this->uart[UART0]->RBR & 0xFF;

	//call receive callback if it has been registered
	if (this->callback[UART0]) {
		this->callback[UART0](__uw_get_user_ptr(), received_char);
	}
	// call terminal character process function
	__uw_terminal_process_rx_msg(&received_char, 1, STDOUT_UART);

}
#elif defined(LPC1785)
#error "LPC1785 missing uart interrupt routine declarations"
#endif


static bool check_uart(uw_uarts_e uart) {
	if (uart >= UART_COUNT) {
		printf("Warning: UART %u not available on %s hardware\n\r", uart, uw_get_hardware_name());
		return false;
	}
	return true;
}


bool uw_uart_add_callback(uw_uarts_e uart, void (*callback_function)(void* user_ptr, char chr)) {
	if (!check_uart(uart)) return false;
	this->callback[uart] = callback_function;
	return true;
}


bool uw_uart_init(uw_uarts_e uart, uint32_t baudRate) {
#ifdef LPC11C14
	this->callback[UART0] = 0;
	this->uart[UART0] = LPC_UART;

	if (!check_uart(uart)) return false;

	//set the baudrate
	uint32_t uart_clock_div = SystemCoreClock / (baudRate * 16);
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
	this->uart[UART0]->IER |= (0x01 << 0);

	/* Configure UART0 */
	this->uart[UART0]->LCR = 0x03;
	//baud rate generation: baud_rate = clk / (16 * (U0DLM >> 8 + U0DLL))
	//from that: U0DLM + U0DLL = clk / (16 * baud_rate)
//	this->uart[UART0]->DLL = clk / (16 * baudRate);
//	this->uart[UART0]->DLM = (clk / (16 * baudRate)) >> 8;

	//reset UART FIFOs
	this->uart[UART0]->FCR |= (0x01 << 1) | (0x01 << 2);

	/* Enable UART0 */
	this->uart[UART0]->TER |= (0x01 << 7);
	this->uart[UART0]->FCR |= 0x01;
#elif defined(LPC1785)
#error "LPC1785 uart init not defined"
	return false;
#endif

	return true;
}


bool uw_uart_config(uw_uarts_e uart, uw_uart_configs_e config) {
	if (!check_uart(uart)) return false;

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
	return true;
#elif defined(LPC1785)
#error "LPC1785 uart config not missing"
	return false;
#endif
}


bool uw_uart_send_char(uw_uarts_e uart, char buffer) {
	if (!check_uart(uart)) return false;

	/* Wait until we're ready to send */
	while (!(this->uart[UART0]->LSR & (1 << 6))) ;
	//send data
	this->uart[UART0]->THR = buffer;

	return true;
}

bool uw_uart_send(uw_uarts_e uart, char *buffer, uint32_t length) {
	if (!check_uart(uart)) return false;

	while (length != 0) {
		uw_uart_send_char(uart, *buffer);
		buffer++;
		length--;
	}
	return true;
}

bool uw_uart_send_str(uw_uarts_e uart, char *buffer) {
	if (!check_uart(uart)) return false;

	while (*buffer != '\0') {
		uw_uart_send_char(uart, *buffer);
		buffer++;
	}

	return true;
}

