/*
 * hal_debug.h
 *
 *  Created on: Nov 13, 2015
 *      Author: usevolt
 */

#ifndef HAL_DEBUG_H_
#define HAL_DEBUG_H_


#include <stdint.h>


/// @brief: Processes rx messages
void hal_debug_process_rx_msg(uint8_t* data, uint8_t data_length);


#endif /* HAL_DEBUG_H_ */
