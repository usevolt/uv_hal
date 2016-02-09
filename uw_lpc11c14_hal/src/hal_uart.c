/*
 * hal_uart_controller.c
 *
 *  Created on: Jan 29, 2015
 *      Author: usenius
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "hal_uart_controller.h"
#include "hal_iap_controller.h"
#include "hal_terminal.h"

static void (*callback)(char) = NULL;

//UART interrupt handler
void UART_IRQHandler (void) {
	char received_char = LPC_UART->RBR & 0xFF;

	//call receive callback if it has been registered
	if (callback) {
		callback(received_char);
	}
	// call terminal character process function
	__hal_terminal_process_rx_msg(&received_char, 1, STDOUT_UART);

}


void hal_uart0_register_callback(void (*callback_function)(char chr)) {
	callback = callback_function;
}


void hal_init_uart0(uint32_t baudRate) {

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
	LPC_UART->IER |= (0x01 << 0);

	/* Configure UART0 */
	LPC_UART->LCR = UART_DATA_LENGTH_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
	//baud rate generation: baud_rate = clk / (16 * (U0DLM >> 8 + U0DLL))
	//from that: U0DLM + U0DLL = clk / (16 * baud_rate)
//	LPC_UART->DLL = clk / (16 * baudRate);
//	LPC_UART->DLM = (clk / (16 * baudRate)) >> 8;

	//reset UART FIFOs
	LPC_UART->FCR |= (0x01 << 1) | (0x01 << 2);

	/* Enable UART0 */
	LPC_UART->TER |= (0x01 << 7);
	LPC_UART->FCR |= 0x01;
}

void hal_uart0_send_char(char buffer) {
	/* Wait until we're ready to send */
	while (!(LPC_UART->LSR & (1 << 6))) ;
	//send data
	LPC_UART->THR = buffer;
}

void hal_uart0_send(char *buffer, uint32_t length) {
	while (length != 0) {
		hal_uart0_send_char(*buffer);
		buffer++;
		length--;
	}
}

void hal_uart0_send_str(char *buffer) {
	while (*buffer != '\0') {
		hal_uart0_send_char(*buffer);
		buffer++;
	}
}

