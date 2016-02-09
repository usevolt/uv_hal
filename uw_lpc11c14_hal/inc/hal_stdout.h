/*
 * hal_putchar.h
 *
 *  Created on: Apr 26, 2015
 *      Author: usevolt
 */

#ifndef HAL_STDOUT_H_
#define HAL_STDOUT_H_

/// @brief: Defines the standard output for printf and such


typedef enum {
	STDOUT_UNDEFINED,
	STDOUT_UART,
	STDOUT_CAN
} hal_stdout_sources_e;

/// @brief: Set's putchar's output either UART or CAN
void hal_stdout_set_source(hal_stdout_sources_e value);

/// @brief: Sends a non-terminated string via the standard output
void hal_stdout_send(char* str, unsigned int count);


#endif /* HAL_STDOUT_H_ */
