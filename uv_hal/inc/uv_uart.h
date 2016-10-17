/*
 * uv_uart.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef UW_UART_H_
#define UW_UART_H_


#include "uv_hal_config.h"


#include <stdbool.h>
#include <stdint.h>
#include "uv_stdout.h"
#include "uv_errors.h"

#if CONFIG_UART0 || CONFIG_UART1 || CONFIG_UART2 || CONFIG_UART3
#define CONFIG_UART			1
#endif



#if CONFIG_UART

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
} uv_uarts_e;




/// @brief: initializes the given UART module. UART configurations are specified
/// at compile time in uv_hal_config.h
///
/// @return: uv_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
///
/// @param uart: The hardware uart module which will be initialized
uv_errors_e _uv_uart_init(uv_uarts_e uart);




/// @brief: sends 1 character synchronously.
/// function returns when last character has put into transfer buffer
///
/// @return: uv_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
uv_errors_e uv_uart_send_char 	(uv_uarts_e uart, char buffer);

/// @brief: sends a string synchronously
/// function returns when last character has put into transfer buffer
///
/// @return: uv_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
///
/// @param length How many characters will be sent
uv_errors_e uv_uart_send     	(uv_uarts_e uart, char *buffer, uint32_t length);

/// @brief: sends a string synchronously
/// function returns when last character has put into transfer buffer.
///
/// @return: uv_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
///
/// Sends characters till '\0' is met
/// String to be send MUST be null-terminated string
uv_errors_e uv_uart_send_str		(uv_uarts_e uart, char *buffer);

/// @brief: Gets the oldest received byte from the UART buffer and stores it
/// to 'dest'.
uv_errors_e uv_uart_get_char(uv_uarts_e uart, char *dest);



/// @brief: Registers a receive callback function when anything is received via uart
///
///@note: The callback function is called from the ISR and thus it should be kept
/// as short as possible.
///
/// @return: uv_errors_e describing if an error occurred. If succesful, ERR_NONE is returned.
uv_errors_e uv_uart_add_callback(uv_uarts_e uart,
		void (*callback_function)(void* user_ptr, uv_uarts_e uart, char chr));


/// @brief: Returns whether or not the uart is initialized.
bool uv_uart_is_initialized(uv_uarts_e uart);

#endif

#endif /* UW_UART_H_ */
