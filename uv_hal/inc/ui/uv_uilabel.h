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

#ifndef UV_HAL_INC_UI_UV_UILABEL_H_
#define UV_HAL_INC_UI_UV_UILABEL_H_


#include <ui/uv_uiobject.h>
#include <uv_hal_config.h>
#include <stdlib.h>
#include "uv_utilities.h"
#include "ui/uv_uifont.h"
#include <string.h>


#if CONFIG_UI



/// @brief: Structure for showing text label on the screen.
typedef struct   {
	EXTENDS(uv_uiobject_st);

	/// @brief: Pointer to the font of this label
	uv_font_st *font;
	/// @brief: Text color
	color_t color;
	/// @brief: Null-terminated string which will be shown on the screen
	char *str;
	/// @brief: Label alignment
	alignment_e align;
} uv_uilabel_st;


#undef this
#define this ((uv_uilabel_st*)me)


void uv_uilabel_init(void *me, uv_font_st *font,
		alignment_e alignment, color_t color, char *str);


void _uv_uilabel_draw(void *me, const uv_bounding_box_st *pbb);


#define this		((uv_uilabel_st*)me)


/// @brief: Set's the text of all objects which are inherited from uv_uilabel_st
///
/// @note: Since string are passed as a reference parameters, updating the same text
/// will not refresh the label itself. Thus uv_ui_refresh should be called after updating the text.
void uv_uilabel_set_text(void *me, char *str);


/// @brief: Returns the current text assigned to the uilabel
static inline char *uv_uilabel_get_text(void *me) {
	return this->str;
}


static inline void uv_uilabel_set_font(void *me, uv_font_st *value) {
	this->font = value;
	uv_ui_refresh(this);
}

static inline uv_font_st *uv_uilabel_get_font(void *me) {
	return this->font;
}

/// @brief: Sets the color of the label text
void uv_uilabel_set_color(void *me, color_t c);



#undef this








#define this ((uv_uidigit_st*)me)



/// @brief: Typedef for digit label. Should be used with CONFIG_UI_NUM_XXXXX reduced fonts and
/// they can only contain numbers, comma and periods.
typedef struct {
	EXTENDS(uv_uilabel_st);

	uint32_t divider;
	char str[13];
	char format[10];
	int32_t value;
} uv_uidigit_st;



/// @brief: Initializes the digit label.
///
/// @param format: Printf-format for showing the digit. Without fractions the
/// format should contain only one number. If fractions are used, it should define
/// exactly 2 numbers separated by a dot or a comma.
void uv_uidigit_init(void *me, uv_font_st *font,
		alignment_e alignment, color_t color, char *format, int value);


void uv_uidigit_set_value(void *me, int value);


/// @brief: a way for setting any string to digit. Can be used to disable digit numbers.
void uv_uidigit_set_text(void *me, char *str);


/// @brief: Sets the color of the label text
static inline void uv_uidigit_set_color(void *me, color_t c) {
	uv_uilabel_set_color(me, c);
}


/// @brief: If set to true, uidigit will also show fractions divided by *value*
static inline void uv_uidigit_set_divider(void *me, unsigned int value) {
	if (!value) { value = 1; }
	this->divider = value;
	uv_ui_refresh(this);
}


static inline int uv_uidigit_get_value(void *me) {
	return this->value;
}





#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UILABEL_H_ */
