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


#ifndef HAL_RESET_H_
#define HAL_RESET_H_


#include "uv_hal_config.h"

#include <stdbool.h>

typedef enum {
	UV_RESET_POR = 0,
	UV_RESET_EXTERNAL,
	UV_RESET_WATCHDOG,
	UV_RESET_BROWN_OUT,
	UV_RESET_SOFTWARE,
	UV_RESET_LOCKUP,
	UV_RESET_COUNT
} uv_reset_sources_e;


/// @brief: resets the system via software reset
void uv_system_reset();


/// @brief: Starts the uv bootloader. As bootloader starts when the device resets,
/// this effectively just resets the device without clearing
/// the shared RAM memory for the uv bootloader.
void uv_bootloader_start();

#endif /* HAL_RESET_H_ */
