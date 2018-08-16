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


#ifndef UV_HAL_INC_UI_UV_UINUMPAD_H_
#define UV_HAL_INC_UI_UV_UINUMPAD_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_ui.h"

#if CONFIG_UI

#define NUMPAD_VALUE_STR_LEN	16


typedef struct {
	EXTENDS(uv_uiobject_st);

	char value_str[NUMPAD_VALUE_STR_LEN];
	int32_t value;
	const char *title;
	const uv_uistyle_st *style;
	bool submitted;
	bool cancelled;
	int8_t pressed_index;
	int8_t released_index;

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
int32_t uv_uinumpaddialog_exec(const char *title, int32_t def_value, const uv_uistyle_st *style);

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

#undef this

#endif



#endif /* UV_HAL_INC_UI_UV_UINUMPAD_H_ */
