/*
 * uw_uart.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef UW_UART_H_
#define UW_UART_H_


#include "uw_hal_config.h"


#include <stdbool.h>
#include <stdint.h>
#include "uw_stdout.h"
#include "uw_errors.h"


/// @brief: Defines UARTS usable on the target system
/// @note: All systems must define UART_COUNT member which defines the maximum number of UARTs.
typedef enum {
#if CONFIG_TARGET_LPC11C14
#if CONFIG_UART0
	UART0		= 0,
#endif
	UART_COUNT 	= 1
#elif CONFIG_TARGET_LPC1785
#if CONFIG_UART0
	UART0 		= 0,
#endif
#if CONFIG_UART1
	UART1 		= 1,
#endif
#if CONFIG_UART2
	UART2 		= 2,
#endif
#if CONFIG_UART3
	UART3 		= 3,
#endif
#if CONFIG_UART4
	UART4 		= 4,
#endif
	UART_COUNT 	= 5
#endif
} uw_uarts_e;




/// @brief: initializes the given UART module. UART configurations are specified
/// at compile time in uw_hal_config.h
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
///
/// @param uart: The hardware uart module which will be initialized
uw_errors_e uw_uart_init(uw_uarts_e uart);




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

/// @brief: Gets the oldest received byte from the UART buffer and stores it
/// to 'dest'.
uw_errors_e uw_uart_get_char(uw_uarts_e uart, char *dest);



/// @brief: Registers a receive callback function when anything is received via uart
///
///@note: The callback function is called from the ISR and thus it should be kept
/// as short as possible.
///
/// @return: uw_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
uw_errors_e uw_uart_add_callback(uw_uarts_e uart,
		void (*callback_function)(void* user_ptr, uw_uarts_e uart, char chr));


/// @brief: Returns whether or not the uart is initialized.
bool uw_uart_is_initialized(uw_uarts_e uart);


#endif /* UW_UART_H_ */
