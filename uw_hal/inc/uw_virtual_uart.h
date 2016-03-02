/*
 * uw_virtual_uart.h
 *
 *  Created on: Mar 2, 2016
 *      Author: usevolt
 */

#ifndef UW_VIRTUAL_UART_H_
#define UW_VIRTUAL_UART_H_


#include <stdint.h>
#include "uw_errors.h"
#include "uw_utilities.h"

/// @file: Virtual uart is a bit-banged uart. There can be as many virtual uarts
/// as there are enough pins or timers in the controller,
/// but they consume processing power and the maximum baudrate is pretty low.
///
/// To use a virtual uart, create a uw_virtual_uart_st object, init it and
/// add callbacks for IO-pin and timer used in virtual uart.
/// Then call uw_virtual_uart_process_rx in both of those callback functions. That's all.
///
/// IMPORTANT: uw_timer_add_callback must be called after uw_virtual_uart_init function.
/// This is because timer init function must be called once and only once per application
/// and before adding any callbacks to timers. uw_virtual_uart_init calls uw_timer_init.
///
/// @example:
///
/// uw_virtual_uart_st uart;
/// uw_virtual_uart_init(&uart, PIO1_3, PIO1_4, TIMER0, 9600, callback);
/// uw_gpio_add_callback(gpio_callback);
/// uw_timer_add_callback(TIMER0, timer_callback);
///
/// void gpio_callback(uw_gpios_e pin, ...) {
///		if (pin == uart->rx_io) {
///			uw_virtual_uart_process_rx(&uart);
///		}
///}
///
/// void timer_callback(uw_timers_e timer, ...) {
///		if (timer == uart->timer) {
///			uw_virtual_uart_process_rx(&uart);
///		}
/// }



#define _uw_timers_e uint16_t
#define _uw_gpios_e uint32_t
typedef struct {
	/// @brief: A uw_gpios_e variable describing the PIN used as a transmit line
	_uw_gpios_e tx_io;
	/// @brief: A uw_gpios_e variable describing the PIN used as a receive line
	_uw_gpios_e rx_io;
	/// @brief: A uw_timers_e variable describing the timer which is used for calculations
	/// @note: Timer should be uninitialized! Virtual UART takes a full ownership of the
	/// timer and reinitializing it afterwards result in a hard fault interrupt.
	_uw_timers_e timer;
	/// @brief: Desired baudrate. no greater than 9600 baudrate should be used. It might
	/// be possible though...
	uint16_t baudrate;
	/// @brief: Receive callback function pointer. This function will be called when
	/// data is received. If this is not used, NULL can be assigned.
	/// @param user_ptr: A user specified application pointer. See uw_utilities.h for details.
	void (*rx_callback)(void *user_ptr, char);
	/// @brief: Indicates if a byte is being received. This is put to true after start bit
	/// and cleared after 8 bits have been received.
	bool receiving;
	char byte;
	uint8_t bits;
} uw_virtual_uart_st;



/// @brief: Initializes a virtual uart.
///
/// @note: Virtual uart is a bit-banged uart. There can be as many virtual uarts
/// as there are enough pins or timers in the controller,
/// but they consume processing power and the maximum baudrate is pretty low.
/// Right now virtual UART supports only 8n1-protocol. That is, 8 bits, no parity, 1 stop bit.
/// Also no error detection is available. To guarantee the byte receiving,
/// the user application should specify timer's interrupts as high priority as possible.
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
uw_errors_e uw_virtual_uart_init(uw_virtual_uart_st *uart, _uw_gpios_e rx_pin,
		_uw_gpios_e tx_pin, _uw_timers_e timer, uint16_t baudrate,
		void (*callback)(void *user_ptr, char));



/// @brief: Sends a char via virtual uart.
///
/// @note: Function returns when the character was sent. Virtual uarts do not support
/// asynchronous transmissions.
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
uw_errors_e uw_virtual_uart_send(uw_virtual_uart_st *uart, char c);



/// @brief: Sends a null-terminated string via virtual uart.
///
/// @note: Function returns when the whole string was sent. Vitual uarts do not support
/// asynchronous transmissions.
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
uw_errors_e uw_virtual_uart_send_str(uw_virtual_uart_st *uart, char *str);


/// @brief: The user program should call this function when it detects that a byte is being
/// received. This causes the virtual uart to activate and receive the character.
/// Usually this is called in a GPIO interrupt handler as well as timer interrupt handler.
///
/// @return: uw_errors_e describing if an error occurred. If successful, ERR_NONE is returned.
uw_errors_e uw_virtual_uart_process_rx(uw_virtual_uart_st *uart);


/// @brief: Assign's the last received character to c. If no characters are received since the
/// last call to this function, no characters are assigned and function returns an error.
///
/// @return: uw_errors_e describing if an error occurred. If successful, ERR_NONE is returned.
///
/// @param c: Pointer to character where the received value will be saved
uw_errors_e uw_virtual_uart_get_char(uw_virtual_uart_st *uart, char *c);



#endif /* UW_VIRTUAL_UART_H_ */
