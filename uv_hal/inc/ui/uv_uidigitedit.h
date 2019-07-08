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


#ifndef UV_HAL_INC_UI_UV_UIDIGITEDIT_H_
#define UV_HAL_INC_UI_UV_UIDIGITEDIT_H_

#include "uv_utilities.h"
#include "uv_ui.h"


#if CONFIG_UI





/// @brief: Structure for showing text label on the screen.
typedef struct __attribute__((packed)) {
	EXTENDS(uv_uilabel_st);

	char str[16];
	char *title;
	char *numpaddialog_title;
	color_t bg_color;
	int32_t value;
	bool changed;
	const uv_uistyle_st *style;
	int32_t limit_max;
	int32_t limit_min;
} uv_uidigitedit_st;


#undef this
#define this ((uv_uidigitedit_st*)me)


void uv_uidigitedit_init(void *me, uv_font_st *font,
		color_t color, uint32_t value, const uv_uistyle_st *style);


/// @brief: Set's the text of all objects which are inherited from uv_uilabel_st
///
/// @note: Since string are passed as a reference parameters, updating the same text
/// will not refresh the label itself. Thus uv_ui_refresh should be called after updating the text.
void uv_uidigitedit_set_value(void *me, int32_t value);




/// @brief: Sets the color of the label text
static inline void uv_uidigitedit_set_text_color(void *me, color_t c) {
	uv_uilabel_set_color(me, c);
}

static inline void uv_uidigitedit_set_bg_color(void *me, color_t c) {
	this->bg_color = c;
}


/// @brief: Sets the title for the digitedit. The title text is shown below the digitedit fiel
static inline void uv_uidigitedit_set_title(void *me, char *value) {
	this->title = value;
}

static inline char *uv_uidigitedit_get_title(void *me) {
	return this->title;
}

/// @brief: Sets the string for the numpad dialog which opens when the uidigitedit is clicked
static inline void uv_uidigitedit_set_numpad_title(void *me, char *value) {
	this->numpaddialog_title = value;
}

/// @brief: Returns true on the step cycle when the value was changed
static inline bool uv_uidigitedit_value_changed(void *me) {
	return this->changed;
}

static inline int64_t uv_uidigitedit_get_value(void *me) {
	return this->value;
}


/// @brief: Sets the maximum limit
static inline void uv_uidigitedit_set_maxlimit(void *me, int32_t value) {
	this->limit_max = value;
}

/// @brief: Returns the maximum limit
static inline int32_t uv_uidigitedit_get_maxlimit(void *me) {
	return this->limit_max;
}

/// @brief: Sets the negative limit. The limit defaults to 0, in which case
/// the uinumpad wont display sign-button.
static inline void uv_uidigitedit_set_minlimit(void *me, int32_t value) {
	this->limit_min = value;
}



#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIDIGITEDIT_H_ */
