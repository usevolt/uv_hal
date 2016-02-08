/*
 * hal_uart_controller.h
 *
 *  Created on: Jan 29, 2015
 *      Author: usenius
 */



#ifndef HAL_UART_CONTROLLER_H_
#define HAL_UART_CONTROLLER_H_

#include "LPC11xx.h"
#include <stdbool.h>
#include "hal_stdout.h"

#define UART_DATA_LENGTH_8   (0x03 << 0)
#define UART_PARITY_NONE     (0 << 3)
#define UART_STOP_BIT_1      (0 << 2)

/* Status bits */
#define UART_STATUS_RXRDY    (1 << 0)
#define UART_STATUS_RXIDLE   (1 << 1)
#define UART_STATUS_TXRDY    (1 << 2)
#define UART_STATUS_TXIDLE   (1 << 3)
#define UART_STATUS_CTSDEL   (1 << 5)
#define UART_STATUS_RXBRKDEL (1 << 11)


/// @brief: initializes uart0
/// @param baudRate desired baudrate in 1/s
void hal_init_uart0     	(uint32_t baudRate);

/// @brief: sends 1 character synchronously.
/// function returns when last character has put into transfer buffer
void hal_uart0_send_char 	(char buffer);

/// @brief: sends a string synchronously
/// function returns when last character has put into transfer buffer
/// @param length How many characters will be sent
void hal_uart0_send     	(char *buffer, uint32_t length);

/// @brief: sends a string synchronously
/// function returns when last character has put into transfer buffer.
/// Sends characters till '\0' is met
/// String to be send MUST be null-terminated string
void hal_uart0_send_str		(char *buffer);

/// @brief: Registers a receive callback function when anything is received via uart
void hal_uart0_register_callback(void (*callback_function)(char chr));




#endif /* HAL_UART_CONTROLLER_H_ */
