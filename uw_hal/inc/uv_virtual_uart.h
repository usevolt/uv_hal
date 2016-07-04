/*
 * uv_virtual_uart.h
 *
 *  Created on: Mar 2, 2016
 *      Author: usevolt
 */

#ifndef UW_VIRTUAL_UART_H_
#define UW_VIRTUAL_UART_H_


#include "uv_hal_config.h"


#include <stdint.h>
#include "uv_errors.h"
#include "uv_utilities.h"

/// @file: Virtual uart is a bit-banged uart. There can be as many virtual uarts
/// as there are enough pins or timers in the controller,
/// but they consume processing power and the maximum baudrate is pretty low.
///
/// To use a virtual uart, create a uv_virtual_uart_st object, init it and
/// add callbacks for IO-pin and timer used in virtual uart.
/// Then call uv_virtual_uart_process_rx in both of those callback functions. That's all.
///
/// IMPORTANT: uv_timer_add_callback must be called after uv_virtual_uart_init function.
/// This is because timer init function must be called once and only once per application
/// and before adding any callbacks to timers. uv_virtual_uart_init calls uv_timer_init.
///
/// NOTE: Since virtual UART uses only 1 timer to receive and transmit, it's only half-duplex,
/// e.g. it cannot send and receive bytes at the same time. Transmitting is in a higher priority
/// than receiving but transmit will never interrupt a receiving byte.
///
/// NOTE: The virtual uart callback will be called form the ISR. The user program should
/// keep the callback routine as short as possible.
///
/// Transmitting with the virtual UART is asynchronous, but not buffered. This means that
/// one call to uv_virtual_uart_send_str() will return quickly, putting the string
/// to transmit. The UART will transmit bytes asynchronously on the background.
/// However, a second call to that same function will wait for the last transmit to finish
/// before starting to transmit the next string.
///
/// @example:
///
/// uv_virtual_uart_st uart;
/// uv_virtual_uart_init(&uart, PIO1_3, PIO1_4, TIMER0, 9600, callback);
/// uv_gpio_add_callback(gpio_callback);
/// uv_timer_add_callback(TIMER0, timer_callback);
///
/// void gpio_callback(uv_gpios_e pin, ...) {
///		if (pin == uart->rx_io) {
///			uv_virtual_uart_process_rx(&uart);
///		}
///}
///
/// void timer_callback(uv_timers_e timer, ...) {
///		if (timer == uart->timer) {
///			uv_virtual_uart_process_rx(&uart);
///		}
/// }



#define _uv_timers_e uint16_t
#define _uv_gpios_e uint32_t
typedef struct uv_virtual_uart_st uv_virtual_uart_st;
struct uv_virtual_uart_st {
	/// @brief: A uv_gpios_e variable describing the PIN used as a transmit line
	_uv_gpios_e tx_io;
	/// @brief: A uv_gpios_e variable describing the PIN used as a receive line
	_uv_gpios_e rx_io;
	/// @brief: A uv_timers_e variable describing the timer which is used for calculations
	/// @note: Timer should be uninitialized! Virtual UART takes a full ownership of the
	/// timer and reinitializing it afterwards result in a hard fault interrupt.
	_uv_timers_e timer;
	/// @brief: Desired baudrate. no greater than 9600 baudrate should be used. It might
	/// be possible though...
	uint16_t baudrate;
	/// @brief: Receive callback function pointer. This function will be called when
	/// data is received. If this is not used, NULL can be assigned.
	/// @param user_ptr: A user specified application pointer. See uv_utilities.h for details.
	/// @param this: Pointer to this virtual uart module
	void (*rx_callback)(void *user_ptr, uv_virtual_uart_st *this, char);
	/// @brief: String transmit callback function pointer. If assigned, this will be called
	/// when the last byte of a null-terminated string has been transmitted
	/// @param user_ptr: A user specified application pointer. See uv_utilities.h for details.
	/// @param this: Pointer to this virtual uart module
	void (*str_tx_callback)(void *user_ptr, uv_virtual_uart_st *this);
	/// @brief: Indicates if a byte is being received. This is put to true after start bit
	/// and cleared after 8 bits have been received.
	bool receiving;
	bool transmitting;
	char byte;
	// transmit ptr points to the string which will be transmitted
	char *transmit_ptr;
	uint8_t bits;
};



/// @brief: Initializes a virtual uart.
///
/// @note: Virtual uart is a bit-banged uart. There can be as many virtual uarts
/// as there are enough pins or timers in the controller,
/// but they consume processing power and the maximum baudrate is pretty low.
/// Right now virtual UART supports only 8n1-protocol. That is, 8 bits, no parity, 1 stop bit.
/// Also no error detection is available. To guarantee the byte receiving,
/// the user application should specify timer's interrupts as high priority as possible.
///
/// @return: uv_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
uv_errors_e uv_virtual_uart_init(uv_virtual_uart_st *uart, _uv_gpios_e rx_pin,
		_uv_gpios_e tx_pin, _uv_timers_e timer, uint16_t baudrate,
		void (*rx_callback)(void *user_ptr, uv_virtual_uart_st *this, char),
		void (*str_tx_callback)(void *user_ptr, uv_virtual_uart_st *this));



/// @brief: Sends a char via virtual uart.
///
/// @note: Function returns when first bit of this char is started to transmit.
///
/// @return: uv_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
uv_errors_e uv_virtual_uart_send(uv_virtual_uart_st *uart, char *c);



/// @brief: Sends a null-terminated string via virtual uart.
///
/// @note: Function returns when transmitting the string was started. This means that
/// if there is another string in transmit when calling this, the function will wait
/// for the last string to finish transmitting and then return.
///
/// @return: uv_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
uv_errors_e uv_virtual_uart_send_str(uv_virtual_uart_st *uart, char *str);


/// @brief: The user program should call this function in both GPIO and timer interrupt routines.
///
/// @return: uv_errors_e describing if an error occurred. If successful, ERR_NONE is returned.
uv_errors_e uv_virtual_uart_isr(uv_virtual_uart_st *uart);


/// @brief: Assign's the last received character to c. If no characters are received since the
/// last call to this function, no characters are assigned and function returns an error.
///
/// @return: uv_errors_e describing if an error occurred. If successful, ERR_NONE is returned.
///
/// @param c: Pointer to character where the received value will be saved
uv_errors_e uv_virtual_uart_get_char(uv_virtual_uart_st *uart, char *c);



/// @brief: Returns true if uart is ready to send
bool uv_virtual_uart_ready_to_send(uv_virtual_uart_st *uart);

#endif /* UW_VIRTUAL_UART_H_ */
