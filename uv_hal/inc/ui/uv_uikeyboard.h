/*
 * uv_uikeyboard.h
 *
 *  Created on: Oct 27, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UIKEYBOARD_H_
#define UV_HAL_INC_UI_UV_UIKEYBOARD_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_ui.h"

#if CONFIG_LCD


/// @brief: Shows the touchscreen keyboard. The function returns when the user has
/// finished inputting the text
///
/// @note: The keyboard takes the ownership of the whole LCD screen
///
/// @return: True if the user entered any text, false if keyboard was canceled
///
/// @param title: One line info text which will be shown on top of the text area
/// @param buffer: Buffer where the inputted text will be saved
/// @param buf_len: The maximum length of the buffer
/// @param style: The UI style of the keyboard
bool uv_uikeyboard_show(const char *title, char *buffer,
		uint16_t buf_len, const uv_uistyle_st *style);



#endif

#endif /* UV_HAL_INC_UI_UV_UIKEYBOARD_H_ */
