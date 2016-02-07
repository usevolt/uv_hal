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

// debug command max size, arguments max count
#define UART_RECEIVE_BUFFER_SIZE		30
#define UART_ARG_COUNT					4


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


/// @brief: registers a receive callback function to be called when
/// uart0 receives a character.
/// @param command String containing received command
/// @param args an array of char pointers containing received arguments.
/// Max number of arguments is defined by UART_ARG_COUNT. All unused arguments
/// will point to '\0' character.
///
/// Uart command interface provides a simple terminal for debugging.
/// Enter finishes a command and backspace erases last entered character from command.
/// Question mark ('?') is a restricted character in normal commands. The MCU will
/// enter ISP mode instantly when '?' is received.
void hal_uart0_register_command_callback(void (*callback_function)(char* command, char** args));


/// @brief: Registers a receive callback function when anything is received via uart
void hal_uart0_register_callback(void (*callback_function)(char chr));


/// @brief: extended implementation of C stdlib's atoi function
/// @param str null-terminated string to be converted into integer
/// @param base number base in which the conversion is done. Valid values are 10 and 16
int hal_atoi(const char* str, int base);


///@brief: Disables or enables ISP mode entry when '?' is received from UART
void hal_uart0_disable_isp_entry(bool value);

/// @brief: Clears the inner buffer for command interface
void hal_uart0_clear_buffer(void);


#endif /* HAL_UART_CONTROLLER_H_ */
