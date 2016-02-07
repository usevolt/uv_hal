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
	HAL_STDOUT_UART = 0,
	HAL_STDOUT_CAN
} hal_stdout_e;

/// @brief: Set's putchar's output either UART or CAN
void hal_set_stdout(hal_stdout_e value);


#endif /* HAL_STDOUT_H_ */
