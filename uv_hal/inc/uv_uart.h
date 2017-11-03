/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UW_UART_H_
#define UW_UART_H_


#include "uv_hal_config.h"


#include <stdbool.h>
#include <stdint.h>
#include "uv_stdout.h"
#include "uv_errors.h"
#if CONFIG_TARGET_LPC1549
#include "chip.h"
#endif

#if CONFIG_UART0 || CONFIG_UART1 || CONFIG_UART2 || CONFIG_UART3
#define CONFIG_UART			1
#endif

#if CONFIG_UART


#if CONFIG_TARGET_LPC1549
#if CONFIG_UART0
#if !defined(CONFIG_UART0_TX_PIN)
#error "CONFIG_UART0_TX_PIN should be defined as uv_gpio_e value for which pin UART transmit will be assigned"
#endif
#if !defined(CONFIG_UART0_RX_PIN)
#error "CONFIG_UART0_RX_PIN should be defined as uv_gpio_e value for which UART receive will be assigned"
#endif
#endif
#if CONFIG_UART1
#if !defined(CONFIG_UART1_TX_PIN)
#error "CONFIG_UART1_TX_PIN should be defined as uv_gpio_e value for which pin UART transmit will be assigned"
#endif
#if !defined(CONFIG_UART1_RX_PIN)
#error "CONFIG_UART1_RX_PIN should be defined as uv_gpio_e value for which UART receive will be assigned"
#endif
#endif
#if CONFIG_UART2
#if !defined(CONFIG_UART2_TX_PIN)
#error "CONFIG_UART2_TX_PIN should be defined as uv_gpio_e value for which pin UART transmit will be assigned"
#endif
#if !defined(CONFIG_UART2_RX_PIN)
#error "CONFIG_UART2_RX_PIN should be defined as uv_gpio_e value for which UART receive will be assigned"
#endif
#endif
#endif

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
#elif CONFIG_TARGET_LPC1549
#if CONFIG_UART0
	UART0 = (uint32_t) LPC_USART0,
#endif
#if CONFIG_UART1
	UART1 = (uint32_t) LPC_USART1,
#endif
#if CONFIG_UART2
	UART2 = (uint32_t) LPC_USART2,
#endif
	UART_COUNT = 3
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
void uv_uart_add_callback(uv_uarts_e uart,
		void (*callback_function)(void* user_ptr, uv_uarts_e uart));


#endif

#endif /* UW_UART_H_ */
