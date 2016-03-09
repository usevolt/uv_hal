/*
 * uw_uart.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef UW_UART_H_
#define UW_UART_H_

#include <stdbool.h>
#include <stdint.h>
#include "uw_stdout.h"
#include "uw_errors.h"
#include "uw_gpio.h"


/// @brief: Defines UARTS usable on the target system
/// @note: All systems must define UART_COUNT member which defines the maximum number of UARTs.
typedef enum {
#ifdef LPC11C14
	UART0,
	UART_COUNT
#elif defined(LPC1785)
	UART0,
	UART1,
	UART2,
	UART3,
	UART4,
	UART_COUNT
#endif
} uw_uarts_e;


/// @brief: Defines different configuration for uarts
typedef enum {
	UART_DATA_LENGTH_5 = 	(1 << 0),
	UART_DATA_LENGTH_6 = 	(1 << 1),
	UART_DATA_LENGTH_7 = 	(1 << 2),
	UART_DATA_LENGTH_8 = 	(1 << 3),
	UART_STOP_BITS_1 = 		(1 << 4),
	UART_STOP_BITS_2 = 		(1 << 5),
	UART_PARITY_NONE = 		(1 << 6),
	UART_PARITY_ODD = 		(1 << 7),
	UART_PARITY_EVEN = 		(1 << 8)
} uw_uart_configs_e;




#ifdef LPC11C14
/// @brief: initializes uart with a common 8n1 protocol. To init with a different
/// protocol, call uw_uart_config after this.
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
///
/// @param uart: The hardware uart module which will be initialized
/// @param baud_rate: desired baud rate in 1/s
uw_errors_e uw_uart_init(uw_uarts_e uart, uint32_t baud_rate);

#elif defined(LPC1785)
/// @brief: Defines pins available as UART rx and tx pins
typedef enum {
	UART0_TX_PIO0_0 	= PIO0_0,
	UART0_TX_PIO0_2 	= PIO0_2,
	UART1_TX_PIO0_15	= PIO0_15,
	UART1_TX_PIO2_0 	= PIO2_0,
	UART1_TX_PIO3_16 	= PIO3_16,
	UART2_TX_PIO0_10 	= PIO0_10,
	UART2_TX_PIO2_8 	= PIO2_8,
	UART2_TX_PIO4_22 	= PIO4_22,
	UART3_TX_PIO0_0 	= PIO0_0,
	UART3_TX_PIO0_2 	= PIO0_2,
	UART3_TX_PIO4_28 	= PIO4_28,
	UART4_TX_PIO0_22 	= PIO0_22,
	UART4_TX_PIO1_29 	= PIO1_29,
	UART4_TX_PIO5_4 	= PIO5_4,

	UART0_RX_PIO0_1 	= PIO0_1,
	UART0_RX_PIO0_3 	= PIO0_3,
	UART1_RX_PIO0_16 	= PIO0_16,
	UART1_RX_PIO2_1 	= PIO2_1,
	UART1_RX_PIO3_17 	= PIO3_17,
	UART2_RX_PIO0_11 	= PIO0_11,
	UART2_RX_PIO2_9 	= PIO2_9,
	UART2_RX_PIO4_23 	= PIO4_23,
	UART3_RX_PIO0_1 	= PIO0_1,
	UART3_RX_PIO0_3 	= PIO0_3,
	UART3_RX_PIO4_29 	= PIO4_29,
	UART4_RX_PIO2_9 	= PIO2_9,
	UART4_RX_PIO5_3 	= PIO5_3
} uw_uart_pins_e;

/// @brief: initializes uart with a common 8n1 protocol. To init with a different
/// protocol, call uw_uart_config after this.
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
///
/// @param uart: The hardware uart module which will be initialized
/// @param baud_rate: desired baud rate in 1/s
uw_errors_e uw_uart_init(uw_uarts_e uart, uint32_t baud_rate,
		uw_uart_pins_e tx_pin, uw_uart_pins_e rx_pin);
#endif



/// @brief: Configures uart
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
///
/// @note: Calling uw_uart_init initializes uart with 8n1 ( 8 data bytes,
/// no parity, 1 stop bit). If any other configuration is needed, this
/// function should be called.
///
/// @param configurations: OR'red uw_uart_configs_e conf options for byte length,
/// parity, and stop bits. The default without calling this function is 8n1.
uw_errors_e uw_uart_config(uw_uarts_e uart, uw_uart_configs_e configurations);



/// @brief: sends 1 character synchronously.
/// function returns when last character has put into transfer buffer
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
uw_errors_e uw_uart_send_char 	(uw_uarts_e uart, char buffer);

/// @brief: sends a string synchronously
/// function returns when last character has put into transfer buffer
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
///
/// @param length How many characters will be sent
uw_errors_e uw_uart_send     	(uw_uarts_e uart, char *buffer, uint32_t length);

/// @brief: sends a string synchronously
/// function returns when last character has put into transfer buffer.
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
///
/// Sends characters till '\0' is met
/// String to be send MUST be null-terminated string
uw_errors_e uw_uart_send_str		(uw_uarts_e uart, char *buffer);

/// @brief: Registers a receive callback function when anything is received via uart
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
///
uw_errors_e uw_uart_add_callback(uw_uarts_e uart,
		void (*callback_function)(void* user_ptr, uw_uarts_e uart, char chr));


/// @brief: Returns whether or not the uart is initialized.
bool uw_uart_is_initialized(uw_uarts_e uart);


#endif /* UW_UART_H_ */
