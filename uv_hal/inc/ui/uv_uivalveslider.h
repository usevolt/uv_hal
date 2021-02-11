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
#ifndef HAL_UV_HAL_INC_UI_UV_UIVALVESLIDER_H_
#define HAL_UV_HAL_INC_UI_UV_UIVALVESLIDER_H_

#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_ui.h"

#if CONFIG_UI


#ifdef this
#undef this
#endif
#define this ((uv_uivalveslider_st*) me)


#ifndef CONFIG_UIVALVESLIDER_HANDLE_WIDTH
// Defines the uivalveslider handle width in pixels
#define CONFIG_UIVALVESLIDER_HANDLE_WIDTH	40
#endif


typedef enum {
	UIVALVESLIDER_HANDLE_POS_MIN = 0,
	UIVALVESLIDER_HANDLE_POS_MAX,
	UIVALVESLIDER_HANDLE_NEG_MIN,
	UIVALVESLIDER_HANDLE_NEG_MAX,
	UIVALVESLIDER_HANDLE_COUNT
} uv_uivalveslider_handles_e;



/// @brief: uivalveslider is a multihandle slider that is used for setting
/// the min & max currents for a unidir or dual dir solenoid output
typedef struct __attribute__((packed)) {
	EXTENDS(uv_uiobject_st);

	int16_t min_val;
	int16_t max_val;
	int16_t inc_step;
	uv_uivalveslider_handles_e selected_handle;
	bool value_changed;
	bool unidir;
	uint8_t horiz_padding;
	int16_t drag_start_val;
	int16_t drag_x;

	uv_uimedia_st *leftarrow_media;
	uv_uimedia_st *rightarrow_media;
	char *handle_strs[UIVALVESLIDER_HANDLE_COUNT];
	int16_t handle_values[UIVALVESLIDER_HANDLE_COUNT];
	int16_t cursor_position;
	char *cursor_text;
	char *title;

	color_t negative_c;
	color_t positive_c;
	color_t outbounds_c;
	uv_font_st *font;
	color_t handle_c;
	color_t selected_handle_c;
	color_t text_c;
	uv_delay_st inc_delay;
	bool inc;

} uv_uivalveslider_st;


/// @brief: Initializes the uivalveslider. If the **min_val** is positive,
/// the valveslider works in unidirectional mode.
///
/// @param leftarrow_media: If not NULL, this media will be shown on the
/// left side of the selected handle
/// @param rightarrow_media: If not NULL, this media will be shown on the
/// right side of the selected handle
/// @param handle_strs: Array of strings that are used as handle titles.
/// UIVALVESLIDER_HANDLE_COUNT length array expected.
/// @param handle_values: Array of int16_t containing the initial values for
/// the handles
void uv_uivalveslider_init(void *me, int16_t min_val, int16_t max_val,
		uv_uimedia_st *leftarrow_media, uv_uimedia_st *rightarrow_media,
		char *handle_strs[], int16_t handle_values[],
		const uv_uistyle_st *style);




/// @brief: Returns true for 1 step cycle when any handle's value has been changed
static inline bool uv_uivalveslider_value_changed(void *me) {
	return this->value_changed;
}


static inline void uv_uivalveslider_set_title(void *me, char *title) {
	this->title = title;
}

/// @brief: Returns the index of the currently selected handle. Only 1 slider can be active
/// at the given time. If no slider is active, returns UIVALVESLIDER_HANDLE_COUNT.
static inline uv_uivalveslider_handles_e uv_uivalveslider_get_selected_handle(void *me) {
	return this->selected_handle;
}


/// @brief: Sets the cursor position and the shown text. The text has to point to a volatile
/// memory address.
static inline void uv_uivalveslider_set_cursor(void *me, int16_t position, char *text) {
	this->cursor_position = position;
	this->cursor_text = text;
}


/// @brief: If set as unidir, the negative side is disabled and it will correspond
/// to the positive side.
static inline void uv_uivalveslider_set_unidir(void *me, bool value) {
	this->unidir = value;
}


static inline bool uv_uivalveslider_get_unidir(void *me) {
	return this->unidir;
}


/// @brief: Sets the horizontal padding. Horizontal padding is free space on left and right
/// edges that is still considered as touch area, but where the sliders do not extend.
static inline void uv_uivalveslider_set_horiz_padding(void *me, uint8_t value) {
	this->horiz_padding = value;
}


/// @brief: Getter for the horizontal padding
static inline uint8_t uv_uivalveslider_get_horiz_padding(void *me) {
	return this->horiz_padding;
}

/// @brief: Sets the currently active handle
static inline void uv_uivalveslider_set_selected_handle(void *me,
		uv_uivalveslider_handles_e value) {
	this->selected_handle = value;
}


/// @brief: Returns the value of the handle specified by **handle**
///
/// @note: **handle** should not over-index
static inline int16_t uv_uivalveslider_get_handle_value(void *me,
		uv_uivalveslider_handles_e handle) {
	return ((uv_uivalveslider_st*) me)->handle_values[handle];
}


/// @brief: Returns the selected handle's value or 0 if no handle is selected
int16_t uv_uivalveslider_get_selected_handle_value(void *me);



/// @brief: Sets the color for the negative side of the slider
static inline void uv_uivalveslider_set_negative_c(void *me, color_t c) {
	this->negative_c = c;
}


/// @brief: Getter for the negative color
static inline color_t uv_uivalveslider_get_negative_c(void *me) {
	return this->negative_c;
}


/// @brief: Sets the color for the positive side of the slider
static inline void uv_uivalveslider_set_positive_c(void  *me, color_t c) {
	this->positive_c = c;
}


/// @brief: Getter for the positive color
static inline color_t uv_uivalveslider_get_positive_c(void *me) {
	return this->positive_c;
}


/// @brief: Sets the font for the valveslider
static inline void uv_uivalveslider_set_font(void *me, uv_font_st *font) {
	this->font = font;
}

/// @brief: Getter for the font
static inline uv_font_st *uv_uivalveslider_get_font(void *me) {
	return this->font;
}


static inline void uv_uivalveslider_set_inc_step(void *me, uint16_t value) {
	this->inc_step = value;
}

static inline uint16_t uv_uivalveslider_get_inc_step(void *me) {
	return this->inc_step;
}


/// @brief: Setter for the left media
static inline void uv_uivalveslider_set_leftarrow_media(void *me, uv_uimedia_st *media) {
	this->leftarrow_media = media;
}

/// @brief: Getter for the left media
static inline uv_uimedia_st *uv_uivalveslider_get_leftarrow_media(void *me) {
	return this->leftarrow_media;
}


/// @brief: Setter for the right media
static inline void uv_uivalveslider_set_rightarrow_media(void *me, uv_uimedia_st *media) {
	this->rightarrow_media = media;
}


/// @brief: Getter fot he right media
static inline uv_uimedia_st *uv_uivalveslider_get_rightarrow_media(void *me) {
	return this->rightarrow_media;
}


#undef this

#endif

#endif /* HAL_UV_HAL_INC_UI_UV_UIVALVESLIDER_H_ */
