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

#ifndef UV_HAL_INC_UI_UV_UIPROGRESSBAR_H_
#define UV_HAL_INC_UI_UV_UIPROGRESSBAR_H_


#include <uv_hal_config.h>
#include <uv_ui.h>
#include <uv_utilities.h>


#if CONFIG_UI

#if !CONFIG_UI_PROGRESSBAR_WIDTH
#error "CONFIG_UI_PROGRESSBAR_WIDTH should define the width of individual\
 progressbar's bars in pixels."
#endif
#if !CONFIG_UI_PROGRESSBAR_SPACE
#error "CONFIG_UI_PROGRESSBAR_SPACE should define the space between individual\
 progressbar's bars in pixels."
#endif
#if !CONFIG_UI_PROGRESSBAR_HEIGHT
#error "CONFIG_UI_PROGRESSAR_HEIGHT should define the height of a (horizontal) progressbar"
#endif


typedef struct   {
	EXTENDS(uv_uiobject_st);

	/// @brief: Current value
	int16_t value_max;
	// minimum value, if progressbar is used in dual mode
	int16_t value_min;
	// cursor value is shown as white bar
	int16_t cursor_val;
	/// @brief: Minimum (zero) value
	int16_t limit_min;
	/// @brief: Maximum value
	int16_t limit_max;
	/// @brief: If set to true, this progress bar will be shown horizontally.
	/// Default to true.
	bool horizontal;

	/// @brief: Optional second color which is shown when the value is below *limit*
	color_t limit_c;
	color_t main_c;
	color_t bg_c;
	color_t text_c;
	uv_font_st *font;

	/// @brief: Optional title text
	char *title;
} uv_uiprogressbar_st;

#ifdef this
#undef this
#endif
#define this ((uv_uiprogressbar_st *)me)

/// @brief: Initializes the progress bar as horizontal bar
void uv_uiprogressbar_init(void *me, int16_t limit_min,
		int16_t limit_max, const uv_uistyle_st *style);


/// @brief: Displays the progressbar as horizontal. This is the default.
static inline void uv_uiprogressbar_set_horizontal(void *me) {
	this->horizontal = true;
	uv_ui_refresh_parent(this);
}

/// @brief: Displays the progressbar as vertical
static inline void uv_uiprogressbar_set_vertical(void *me) {
	this->horizontal = false;
	uv_ui_refresh_parent(this);
}

/// @brief: Sets the progressbar current value
///
/// @param value_max: Right edge of the bar
/// @param value_min: Left edge of the bar
void uv_uiprogressbar_set_value(void *me, int16_t value_min, int16_t value_max);


void uv_uiprogressbar_set_cursor(void *me, int16_t val);

/// @brief: Getter for the value
static inline int16_t uv_uiprogressbar_get_value_max(void *me) {
	return this->value_max;
}

/// @brief: Getter for the value
static inline int16_t uv_uiprogressbar_get_value_min(void *me) {
	return this->value_min;
}


/// @brief: Sets the optional title text. Title is shown below the progressbar
static inline void uv_uiprogressbar_set_title(void *me, char *title) {
	this->title = title;
}

static inline char *uv_uiprogressbar_get_title(void *me) {
	return this->title;
}


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIPROGRESSBAR_H_ */
