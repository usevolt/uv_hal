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

#ifndef UW_STDOUT_H_
#define UW_STDOUT_H_


#include "uv_hal_config.h"

/// @brief: Defines the standard output for printf and such



/// @brief: Sends a non-terminated string via the standard output
void uv_stdout_send(char* str, unsigned int count);


#if CONFIG_TERMINAL_CAN
/// @brief: Will be called in the HAL task function.
void _uv_stdout_hal_step(unsigned int step_ms);
#endif

#endif /* UW_STDOUT_H_ */
