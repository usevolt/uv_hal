/*
 * hal_uart_controller.c
 *
 *  Created on: Jan 29, 2015
 *      Author: usenius
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "hal_uart_controller.h"
#include "hal_iap_controller.h"



/// whole command receive buffer
static char uart_receive_buffer[UART_RECEIVE_BUFFER_SIZE];
/// argument pointers
static char* args[UART_ARG_COUNT];
static uint8_t receive_buffer_index = 0;
static void (*receive_callback_function)(char*, char**) = NULL;
static void (*callback)(char) = NULL;
static bool disable_isp_entry = false;

//UART interrupt handler
void UART_IRQHandler (void) {
	//check for buffer overflow
	if (receive_buffer_index >= UART_RECEIVE_BUFFER_SIZE - 1) {
		int i;
		for (i = 0; i < UART_RECEIVE_BUFFER_SIZE; i++) {
			uart_receive_buffer[i] = '\0';
		}
		receive_buffer_index = 0;
	}

	char received_char = LPC_UART->RBR & 0xFF;

	//echo back received character
	hal_uart0_send_char(received_char);

	//call receive callback if it has been registered
	if (callback) {
		callback(received_char);
	}

	//if questionmark was received, enter isp mode
	if (received_char == '?' && !disable_isp_entry) {
		hal_enter_ISP_mode();
	}

	// if backspace was received, delete last saved character
	if (received_char == 0x08) {
		if (receive_buffer_index > 0) {
			receive_buffer_index--;
		}
		uart_receive_buffer[receive_buffer_index] = '\0';
		return;
	}

	//if carriage return was received, read command, and clear buffer
	if (received_char == 0x0D) {
		int i;
		int p = 0;
		//change line
		hal_uart0_send_char('\n');
		receive_buffer_index = 0;
		for (i = 0; i < UART_RECEIVE_BUFFER_SIZE - 1; i++) {
			// on '\0' command has ended
			if (uart_receive_buffer[i] == '\0') {
				break;
			}
			// search for space (argument start)
			else if (uart_receive_buffer[i] == ' ') {
				uart_receive_buffer[i] = '\0';
				if (p < UART_ARG_COUNT) {
					args[p++] = &uart_receive_buffer[i + 1];
				}
			}
		}

		if (receive_callback_function) {
			receive_callback_function(uart_receive_buffer, args);
			// printf command prompt character after callback function has been executed
			hal_uart0_send_char('>');
		}

		for (i = 0; i < UART_RECEIVE_BUFFER_SIZE; i++) {
			uart_receive_buffer[i] = '\0';
		}
		for (i = 0; i < UART_ARG_COUNT; i++) {
			args[i] = &uart_receive_buffer[UART_RECEIVE_BUFFER_SIZE - 1];
		}
		return;
	}
	//save character to buffer
	uart_receive_buffer[receive_buffer_index++] = received_char;
}


void hal_uart0_register_command_callback(void (*callback_function)(char*, char**)) {
	receive_callback_function = callback_function;
}


void hal_uart0_register_callback(void (*callback_function)(char chr)) {
	callback = callback_function;
}


void hal_init_uart0(uint32_t baudRate) {

	//set receive buffer to '\0\'
	int i;
	for (i = 0; i < UART_RECEIVE_BUFFER_SIZE; i++) {
		uart_receive_buffer[i] = '\0';
	}

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

void hal_uart0_disable_isp_entry(bool value) {
	disable_isp_entry = value;
}

void hal_uart0_clear_buffer(void) {
	int i;
	receive_buffer_index = 0;

	for (i = 0; i < UART_RECEIVE_BUFFER_SIZE; i++) {
		uart_receive_buffer[i] = '\0';
	}
	for (i = 0; i < UART_ARG_COUNT; i++) {
		args[i] = &uart_receive_buffer[UART_RECEIVE_BUFFER_SIZE - 1];
	}

}
