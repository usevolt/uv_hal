/*
 * uw_virtual_uart.c
 *
 *  Created on: Mar 2, 2016
 *      Author: usevolt
 */

#include "uw_virtual_uart.h"

#include "uw_timer.h"
#include "uw_gpio.h"
#include <stdbool.h>
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif


uw_errors_e uw_virtual_uart_init(uw_virtual_uart_st *uart, _uw_gpios_e rx_pin,
		_uw_gpios_e tx_pin, _uw_timers_e timer, uint16_t baudrate,
		void (*rx_callback)(void *user_ptr, uw_virtual_uart_st *this, char),
		void (*str_tx_callback)(void *user_ptr, uw_virtual_uart_st *this)) {
	uart->rx_io = rx_pin;
	uart->tx_io = tx_pin;
	uart->rx_callback = rx_callback;
	uart->str_tx_callback = str_tx_callback;
	uart->baudrate = baudrate;
	uart->timer = timer;
	uart->receiving = false;
	uart->byte = 0xFF;
	uart->bits = 0;
	uart->transmit_ptr = NULL;
	uw_timer_init(uart->timer, uart->baudrate);
	uw_err_pass(uw_gpio_init_output(uart->tx_io, true));
	uw_err_pass(uw_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
								INT_FALLING_EDGE));

	return uw_err(ERR_NONE);
}

uw_errors_e uw_virtual_uart_send(uw_virtual_uart_st *uart, char *c) {
	// wait for receiving to finish
	while (uart->receiving) {};
	// disable rx interrupts (virtual uart is half-duplex only)
	uw_err_pass(uw_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
			INT_DISABLE));
	// mark transmit to start
	uart->transmitting = true;
	uart->transmit_ptr = NULL;
	uart->byte = *c;
	uart->bits = 0;
	uw_err_pass(uw_timer_set_freq(uart->timer, uart->baudrate));
	uw_timer_start(uart->timer);
	// start bit is always zero
	uw_err_pass(uw_gpio_set_pin(uart->tx_io, false));

	return ERR_NONE;
}


uw_errors_e uw_virtual_uart_send_str(uw_virtual_uart_st *uart, char *str) {
	// if something was already transmitting, return error
	if (uart->transmitting) {
		__uw_err_throw(ERR_BUSY | HAL_MODULE_VIRTUAL_UART);
	}

	// wait for receiving to finish
	while (uart->receiving) {};

	// disable rx interrupts (virtual uart is half-duplex only)
	uw_err_pass(uw_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
			INT_DISABLE));
	// mark transmit to start
	uart->transmitting = true;
	uart->transmit_ptr = str + 1;
	uart->byte = *str;
	uart->bits = 0;
	uw_err_pass(uw_timer_set_freq(uart->timer, uart->baudrate));
	uw_timer_start(uart->timer);
	// start bit is always zero
	uw_err_pass(uw_gpio_set_pin(uart->tx_io, false));

	return ERR_NONE;
}


uw_errors_e uw_virtual_uart_isr(uw_virtual_uart_st *uart) {
	// receiving timer interrupts
	if (uart->receiving) {
		char bit = uw_gpio_get_pin(uart->rx_io);

		if (uart->bits == 0) {
			// with first byte modify baudrate
			uw_err_pass(uw_timer_set_freq(uart->timer, uart->baudrate));
		}
		// read coming bytes and increase bit counter by 1
		uart->byte |= (bit << uart->bits++);
		// byte read, stop timer and enable interrupts
		if (uart->bits > 7) {
			uw_timer_stop(uart->timer);
			uw_err_pass(uw_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
					INT_EDGE_SENSITIVE | INT_FALLING_EDGE));
			// call receive callback if assigned
			if (uart->rx_callback) {
				uart->rx_callback(__uw_get_user_ptr(), uart, uart->byte);
			}
			uart->receiving = false;
		}
	}
	// if not receiving, this call was caused by GPIO interrupt
	else if (!uart->transmitting) {
		// disable interrupts on this pin
		// interrupts will be enabled once the byte receiving has been completed
		uw_err_pass(uw_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
				INT_DISABLE));
		// start GPS timer with 0.75 * baudrate
		uw_timer_start(uart->timer);
		uw_err_pass(uw_timer_set_freq(uart->timer, 0.7 * uart->baudrate));
		// start the timer
		uw_timer_start(uart->timer);
		uart->bits = 0;
		uart->byte = 0;
		uart->receiving = true;
	}
	// transmitting
	else {
		if (uart->bits < 8) {
			uw_err_pass(uw_gpio_set_pin(uart->tx_io, uart->byte & (1 << uart->bits++)));
		}
		else if (uart->bits < 9) {
			// stop bit
			uw_err_pass(uw_gpio_set_pin(uart->tx_io, true));
			uart->bits++;
		}
		else {
			//check for next bytes to transmit
			if (uart->transmit_ptr && *uart->transmit_ptr != '\0') {
				uart->bits = 0;
				uart->byte = *(uart->transmit_ptr++);
				// start bit of the next byte
				uw_err_pass(uw_gpio_set_pin(uart->tx_io, false));
//				printf("next: %c\n\r", uart->byte);
			}
			else {
				//transmit finished
				uart->transmitting = false;
				uart->transmit_ptr = NULL;
				// stop timer
				uw_timer_stop(uart->timer);
				// enable rx interrupts
				uw_err_pass(uw_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
						INT_EDGE_SENSITIVE | INT_FALLING_EDGE));
				// call callback
				if (uart->str_tx_callback) {
					uart->str_tx_callback(__uw_get_user_ptr(), uart);
				}
			}
		}
		return ERR_NONE;
	}
	return ERR_NONE;
}


uw_errors_e uw_virtual_uart_get_char(uw_virtual_uart_st *uart, char *c) {
	if (uart->receiving) {
		__uw_err_throw(ERR_BUSY | HAL_MODULE_VIRTUAL_UART);
	}
	else if (uart->byte != 0xFF) {
		*c = uart->byte;
		uart->byte = 0xFF;
		return ERR_NONE;
	}
	else {
		__uw_err_throw(ERR_NO_NEW_VALUES | HAL_MODULE_VIRTUAL_UART);
	}
}

bool uw_virtual_uart_ready_to_send(uw_virtual_uart_st *uart) {
	return !uart->transmitting;
}

