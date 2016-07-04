/*
 * uv_virtual_uart.c
 *
 *  Created on: Mar 2, 2016
 *      Author: usevolt
 */

#include "uv_virtual_uart.h"

#include "uv_timer.h"
#include "uv_gpio.h"
#include <stdbool.h>
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif


uv_errors_e uv_virtual_uart_init(uv_virtual_uart_st *uart, _uv_gpios_e rx_pin,
		_uv_gpios_e tx_pin, _uv_timers_e timer, uint16_t baudrate,
		void (*rx_callback)(void *user_ptr, uv_virtual_uart_st *this, char),
		void (*str_tx_callback)(void *user_ptr, uv_virtual_uart_st *this)) {
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
	uv_timer_init(uart->timer, uart->baudrate);
	uv_err_pass(uv_gpio_init_output(uart->tx_io, true));
	uv_err_pass(uv_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
								INT_FALLING_EDGE));

	return uv_err(ERR_NONE);
}

uv_errors_e uv_virtual_uart_send(uv_virtual_uart_st *uart, char *c) {
	// wait for receiving to finish
	while (uart->receiving) {};
	// disable rx interrupts (virtual uart is half-duplex only)
	uv_err_pass(uv_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
			INT_DISABLE));
	// mark transmit to start
	uart->transmitting = true;
	uart->transmit_ptr = NULL;
	uart->byte = *c;
	uart->bits = 0;
	uv_err_pass(uv_timer_set_freq(uart->timer, uart->baudrate));
	uv_timer_start(uart->timer);
	// start bit is always zero
	uv_err_pass(uv_gpio_set_pin(uart->tx_io, false));

	return ERR_NONE;
}


uv_errors_e uv_virtual_uart_send_str(uv_virtual_uart_st *uart, char *str) {
	// if something was already transmitting, return error
	if (uart->transmitting) {
		__uv_err_throw(ERR_BUSY | HAL_MODULE_VIRTUAL_UART);
	}

	// wait for receiving to finish
	while (uart->receiving) {};

	// disable rx interrupts (virtual uart is half-duplex only)
	uv_err_pass(uv_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
			INT_DISABLE));
	// mark transmit to start
	uart->transmitting = true;
	uart->transmit_ptr = str + 1;
	uart->byte = *str;
	uart->bits = 0;
	uv_err_pass(uv_timer_set_freq(uart->timer, uart->baudrate));
	uv_timer_start(uart->timer);
	// start bit is always zero
	uv_err_pass(uv_gpio_set_pin(uart->tx_io, false));

	return ERR_NONE;
}


uv_errors_e uv_virtual_uart_isr(uv_virtual_uart_st *uart) {
	// receiving timer interrupts
	if (uart->receiving) {
		char bit = uv_gpio_get_pin(uart->rx_io);

		if (uart->bits == 0) {
			// with first byte modify baudrate
			uv_err_pass(uv_timer_set_freq(uart->timer, uart->baudrate));
		}
		// read coming bytes and increase bit counter by 1
		uart->byte |= (bit << uart->bits++);
		// byte read, stop timer and enable interrupts
		if (uart->bits > 7) {
			uv_timer_stop(uart->timer);
			uv_err_pass(uv_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
					INT_EDGE_SENSITIVE | INT_FALLING_EDGE));
			// call receive callback if assigned
			if (uart->rx_callback) {
				uart->rx_callback(__uv_get_user_ptr(), uart, uart->byte);
			}
			uart->receiving = false;
		}
	}
	// if not receiving, this call was caused by GPIO interrupt
	else if (!uart->transmitting) {
		// disable interrupts on this pin
		// interrupts will be enabled once the byte receiving has been completed
		uv_err_pass(uv_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
				INT_DISABLE));
		// start GPS timer with 0.75 * baudrate
		uv_timer_start(uart->timer);
		uv_err_pass(uv_timer_set_freq(uart->timer, 0.7 * uart->baudrate));
		// start the timer
		uv_timer_start(uart->timer);
		uart->bits = 0;
		uart->byte = 0;
		uart->receiving = true;
	}
	// transmitting
	else {
		if (uart->bits < 8) {
			uv_err_pass(uv_gpio_set_pin(uart->tx_io, uart->byte & (1 << uart->bits++)));
		}
		else if (uart->bits < 9) {
			// stop bit
			uv_err_pass(uv_gpio_set_pin(uart->tx_io, true));
			uart->bits++;
		}
		else {
			//check for next bytes to transmit
			if (uart->transmit_ptr && *uart->transmit_ptr != '\0') {
				uart->bits = 0;
				uart->byte = *(uart->transmit_ptr++);
				// start bit of the next byte
				uv_err_pass(uv_gpio_set_pin(uart->tx_io, false));
//				printf("next: %c\n\r", uart->byte);
			}
			else {
				//transmit finished
				uart->transmitting = false;
				uart->transmit_ptr = NULL;
				// stop timer
				uv_timer_stop(uart->timer);
				// enable rx interrupts
				uv_err_pass(uv_gpio_init_input(uart->rx_io, PULL_UP_ENABLED | HYSTERESIS_ENABLED,
						INT_EDGE_SENSITIVE | INT_FALLING_EDGE));
				// call callback
				if (uart->str_tx_callback) {
					uart->str_tx_callback(__uv_get_user_ptr(), uart);
				}
			}
		}
		return ERR_NONE;
	}
	return ERR_NONE;
}


uv_errors_e uv_virtual_uart_get_char(uv_virtual_uart_st *uart, char *c) {
	if (uart->receiving) {
		__uv_err_throw(ERR_BUSY | HAL_MODULE_VIRTUAL_UART);
	}
	else if (uart->byte != 0xFF) {
		*c = uart->byte;
		uart->byte = 0xFF;
		return ERR_NONE;
	}
	else {
		__uv_err_throw(ERR_NO_NEW_VALUES | HAL_MODULE_VIRTUAL_UART);
	}
}

bool uv_virtual_uart_ready_to_send(uv_virtual_uart_st *uart) {
	return !uart->transmitting;
}

