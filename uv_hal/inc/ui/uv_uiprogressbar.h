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


enum {
	UI_PROGRESSBAR_LIMIT_NONE = 0,
	UI_PROGRESSBAR_LIMIT_OVER,
	UI_PROGRESSBAR_LIMIT_UNDER
};
typedef uint8_t uiprogressbar_limit_e;

typedef struct __attribute__((packed)) {
	EXTENDS(uv_uiobject_st);

	/// @brief: Current value
	int16_t value;
	/// @brief: Minimum (zero) value
	int16_t min_val;
	/// @brief: Maximum value
	int16_t max_val;
	/// @brief: If set to true, this progress bar will be shown horizontally.
	/// Default to true.
	bool horizontal;
	/// @brief: Specifies if the limit_color is shown when the value
	/// is over or under the limit value
	uiprogressbar_limit_e limit_type;
	/// @brief: When the value is below or over this, active bar color is changed to *low_color*
	int16_t limit;

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
void uv_uiprogressbar_init(void *me, int16_t min_value,
		int16_t max_value, const uv_uistyle_st *style);


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

/// @brief: Sets the progressbar current value.
///
/// @param value: Value from min_val to max_val
void uv_uiprogressbar_set_value(void *me, int16_t value);


/// @brief: Getter for the value
static inline int16_t uv_uiprogressbar_get_value(void *me) {
	return this->value;
}


/// @brief: Sets the optional title text. Title is shown below the progressbar
static inline void uv_uiprogressbar_set_title(void *me, char *title) {
	this->title = title;
}

static inline char *uv_uiprogressbar_get_title(void *me) {
	return this->title;
}

/// @brief: Sets the limit color. When the value is below *limit*,
/// active bar color will be changed to *color*.
///
/// @param type: UIPROGRESSBAR_LIMIT_OVER if the color is shown when the limit is exceeded,
/// UIPROGRESSBAR_LIMIT_BELOW if the color is shown when the limit is undercut
/// @param limit: Limiting value 0...1000
void uv_uiprogressbar_set_limit(void *me, uiprogressbar_limit_e type,
		int16_t limit, color_t color);



#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIPROGRESSBAR_H_ */
