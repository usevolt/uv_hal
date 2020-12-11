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

/// @brief: uivalveslider is a multihandle slider that is used for setting
/// the min & max currents for a unidir or dual dir solenoid output
typedef struct __attribute__((packed)) {
	EXTENDS(uv_uiobject_st);

	int16_t min_val;
	int16_t max_val;
	uint16_t selected_slider;

	uv_uimedia_st *leftarrow_media;
	uv_uimedia_st *rightarrow_media;

	color_t negative_c;
	color_t positive_c;
	uv_font_st *font;
	color_t handle_c;

} uv_uivalveslider_st;


/// @brief: Initializes the uivalveslider.
///
/// @param leftarrow_media: If not NULL, this media will be shown on the
/// left side of the selected handle
/// @param rightarrow_media: If not NULL, this media will be shown on the
/// right side of the selected handle
void uv_uivalveslider_init(void *me, int16_t min_val, int16_t max_val,
		uv_uimedia_st *leftarrow_media, uv_uimedia_st *rightarrowmedia,
		const uv_uistyle_st *style);



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
