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
#ifdef LPC11C14
#include <LPC11xx.h>
#elif defined(LPC1785)

#endif


uw_errors_e uw_virtual_uart_init(uw_virtual_uart_st *uart, _uw_gpios_e rx_pin,
		_uw_gpios_e tx_pin, _uw_timers_e timer, uint16_t baudrate,
		void (*callback)(void *user_ptr, char)) {
	uart->rx_io = rx_pin;
	uart->tx_io = tx_pin;
	uart->rx_callback = callback;
	uart->baudrate = baudrate;
	uart->timer = timer;
	uart->receiving = false;
	uart->byte = 0xFF;
	uart->bits = 0;
	uw_timer_init(uart->timer, uart->baudrate, SystemCoreClock);
	uw_err_pass(uw_gpio_init_output(uart->tx_io, true));
	uw_err_pass(uw_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
								INT_DISABLE));

	return ERR_NONE;
}

uw_errors_e uw_virtual_uart_send(uw_virtual_uart_st *uart, char c) {

	return ERR_NOT_IMPLEMENTED;
}

uw_errors_e uw_virtual_uart_send_str(uw_virtual_uart_st *uart, char *str) {

	return ERR_NOT_IMPLEMENTED;
}


uw_errors_e uw_virtual_uart_process_rx(uw_virtual_uart_st *uart) {

	// if not receiving, this call was caused by GPIO interrupt
	if (!uart->receiving) {
		// disable interrupts on this pin
		// interrupts will be enabled once the byte receiving has been completed
		uw_err_pass(uw_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
				INT_DISABLE));
		// start GPS timer with 0.75 * baudrate
		uw_err_pass(uw_timer_set_freq(uart->timer, 0.75f * uart->baudrate, SystemCoreClock));
		// start the timer
		uw_err_pass(uw_timer_start(uart->timer));
		uart->bits = 0;
		uart->byte = 0;
		uart->receiving = true;
	}
	// otherwise timer caused the interrupt and bit reading is in process
	else {
		char bit = uw_gpio_get_pin(uart->rx_io);

		if (uart->bits == 0) {
			// with first byte modify baudrate
			uw_err_pass(uw_timer_set_freq(uart->timer, uart->baudrate, SystemCoreClock));
			uw_err_pass(uw_timer_start(uart->timer));
		}
		// read coming bytes and increase bit counter by 1
		uart->byte |= (bit << uart->bits++);
		// byte read, stop timer and enable interrupts
		if (uart->bits > 7) {
			uw_err_pass(uw_timer_stop(uart->timer));
			uw_err_pass(uw_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
					INT_EDGE_SENSITIVE | INT_FALLING_EDGE));
			// call receive callback if assigned
			if (uart->rx_callback) {
				uart->rx_callback(__uw_get_user_ptr(), uart->byte);
			}
			uart->receiving = false;
		}
	}
	return ERR_NONE;
}


uw_errors_e uw_virtual_uart_get_char(uw_virtual_uart_st *uart, char *c) {
	if (uart->receiving) {
		return ERR_BUSY;
	}
	else if (uart->byte != 0xFF) {
		*c = uart->byte;
		uart->byte = 0xFF;
		return ERR_NONE;
	}
	else {
		return ERR_NO_NEW_VALUES;
	}
}
