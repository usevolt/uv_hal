/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
