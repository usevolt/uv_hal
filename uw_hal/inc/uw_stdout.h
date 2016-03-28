/*
 * uw_putchar.h
 *
 *  Created on: Apr 26, 2015
 *      Author: usevolt
 */

#ifndef UW_STDOUT_H_
#define UW_STDOUT_H_


#include "uw_hal_config.h"

/// @brief: Defines the standard output for printf and such


typedef enum {
	STDOUT_UNDEFINED,
	STDOUT_UART0,
	STDOUT_CAN
} uw_stdout_sources_e;

/// @brief: Set's putchar's output either UART or CAN
void uw_stdout_set_source(uw_stdout_sources_e value);

/// @brief: Sends a non-terminated string via the standard output
void uw_stdout_send(char* str, unsigned int count);


#endif /* UW_STDOUT_H_ */
