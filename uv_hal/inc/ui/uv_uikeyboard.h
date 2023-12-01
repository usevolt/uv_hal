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

#ifndef UV_HAL_INC_UI_UV_UIKEYBOARD_H_
#define UV_HAL_INC_UI_UV_UIKEYBOARD_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_ui.h"

#if CONFIG_UI



/// @brief: Shows the touchscreen keyboard. The function returns when the user has
/// finished inputting the text
///
/// @note: The keyboard takes the ownership of the whole LCD screen
///
/// @return: True if the user entered any text, false if keyboard was canceled
///
/// @param title: One line info text which will be shown on top of the text area
/// @param buffer: Buffer where the inputted text will be saved. In case if the *buffer*
/// already contains a null-terminated string, that string will be shown on the keyboard
/// at start. Otherwise the buffer will be initialized as a zero-length string.
/// @param buf_len: The maximum length of the buffer
/// @param style: The UI style of the keyboard
bool uv_uikeyboard_show(const char *title, char *buffer,
		uint16_t buf_len, const uv_uistyle_st *style);



#endif

#endif /* UV_HAL_INC_UI_UV_UIKEYBOARD_H_ */
