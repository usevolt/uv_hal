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




/// @brief: initializes uart
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
///
/// @param uart: The hardware uart module which will be initialized
/// @param baudRate desired baudrate in 1/s
uw_errors_e uw_uart_init(uw_uarts_e uart, uint32_t baud_rate);




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
uw_errors_e uw_uart_add_callback(uw_uarts_e uart, void (*callback_function)(void* user_ptr, char chr));


#endif /* UW_UART_H_ */
