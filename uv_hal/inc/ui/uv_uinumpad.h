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


#ifndef UV_HAL_INC_UI_UV_UINUMPAD_H_
#define UV_HAL_INC_UI_UV_UINUMPAD_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_ui.h"

#if CONFIG_UI

#define NUMPAD_VALUE_STR_LEN	16




typedef struct __attribute__((packed)) {
	EXTENDS(uv_uiobject_st);

	char value_str[NUMPAD_VALUE_STR_LEN];
	int32_t value;
	const char *title;
	const uv_uistyle_st *style;
	bool submitted;
	bool cancelled;
	int8_t pressed_index;
	int8_t released_index;
	// 1 or -1 representing the sign of the value
	int8_t sign;
	int32_t limit_max;
	int32_t limit_min;

} uv_uinumpad_st;


/// @brief: Initializes the touchscreen numpad.
///
/// @note: The keyboard takes the ownership of the whole LCD screen
///
/// @return: True if the user entered any text, false if numpad was canceled
///
/// @param window: Pointer to uiwindow on which the numpad is shown
/// @param title: One line info text which will be shown on top of the text area
/// @param dest: Pointer where inputted number is saved
/// @param style: The UI style of the numpad
void uv_uinumpad_init(void *me, const char *title, const uv_uistyle_st *style);

/// @brief: Helper function to show a dialog with a numpad on it.
///
/// @note: This uses lots of stack to store the uielements!
///
/// @return: Value entered into the numpad
int32_t uv_uinumpaddialog_exec(const char *title,
		int32_t max_limit, int32_t min_limit,
		int32_t def_value, const uv_uistyle_st *style);

#ifdef this
#undef this
#endif
#define this ((uv_uinumpad_st*) me)


/// @brief: Returns the current input value. If nothing has been inputted,
/// returns -1.
static inline int32_t uv_uinumpad_get_value(void *me) {
	return this->value;
}

/// @brief: Returns true for 1 step cycle if the user submitted the text
static inline bool uv_uinumpad_get_submitted(void *me) {
	return this->submitted;
}

/// @brief: Returns true for 1 step cycle if the user cancelled inputting the text
static inline bool uv_uinumpad_get_cancelled(void *me) {
	return this->cancelled;
}

static inline void uv_uinumpad_set_maxlimit(void *me, int32_t value) {
	this->limit_max = value;
}

static inline int32_t uv_uinumpad_get_maxlimit(void *me) {
	return this->limit_max;
}

static inline void uv_uinumpad_set_minlimit(void *me, int32_t value) {
	this->limit_min = value;
}

static inline int32_t uv_uinumpad_get_minlimit(void *me) {
	return this->limit_min;
}

#undef this

#endif



#endif /* UV_HAL_INC_UI_UV_UINUMPAD_H_ */
