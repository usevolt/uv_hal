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

#ifndef UV_HAL_INC_UI_UV_UIDUALLABELBUTTON_H_
#define UV_HAL_INC_UI_UV_UIDUALLABELBUTTON_H_

#include <uv_hal_config.h>
#include "ui/uv_uibutton.h"
#include "ui/uv_uilabel.h"
#include "uv_utilities.h"
#if CONFIG_UI



/// @brief: The dual label button is a button with two labels with different fonts
typedef struct   {
	EXTENDS(uv_uibutton_st);

	char *dualtext;
	color_t dualtext_c;
	uv_font_st *dualfont;

} uv_uiduallabelbutton_st;

#ifdef this
#undef this
#endif
#define this ((uv_uiduallabelbutton_st*)me)

/// @brief: Initializes the dual label button
///
/// @param text: The text which is displayed top on the button
///
/// @param dualtext: The text which is displayed bottom on the button
void uv_uiduallabelbutton_init(void *me, char *text,
		char *dualtext, const uv_uistyle_st *style);


/// @brief: Sets the text of the button
static inline void uv_uiduallabelbutton_set_text(void *me, char *text) {
	uv_uibutton_set_text(me, text);
}


/// @brief: Sets the secondary text
void uv_uiduallabelbutton_set_dual_text(void *me, char *dualtext);


/// @brief: Returns the button text
static inline char *uv_uiduallabelbutton_get_text(void *me) {
	return uv_uibutton_get_text(me);
}


static inline char *uv_uiduallabelbutton_get_dualtext(void *me) {
	return this->dualtext;
}


/// @brief: Returns true if the button was clicked
static inline bool uv_uiduallabelbutton_clicked(void *me) {
	return uv_uibutton_clicked(me);
}

/// @brief: Returns true if the button is currently pressed, e.g. the state is either
/// UIDUALLABELBUTTON_PRESSED or UIDUALLABELBUTTON_LONGPRESSED
static inline bool uv_uiduallabelbutton_is_down(void *me) {
	return uv_uibutton_is_down(me);
}

/// @brief: Returns true if the button was long pressed
static inline bool uv_uiduallabelbutton_long_pressed(void *me) {
	return uv_uibutton_long_pressed(me);
}

/// @brief: Sets the main color of the uibutton. The button should be refreshed after
/// calling this.
static inline void uv_uiduallabelbutton_set_main_color(void *me, color_t c) {
	uv_uibutton_set_main_color(me, c);
}

/// @brief: Returns the button main color
static inline color_t uv_uiduallabelbutton_get_main_color(void *me) {
	return uv_uibutton_get_main_color(me);
}


static inline void uv_uiduallabelbutton_set_dual_color(void *me, color_t c) {
	this->dualtext_c = c;
}

static inline color_t uv_uiduallabelbutton_get_dual_color(void *me) {
	return this->dualtext_c;
}


static inline void uv_uiduallabelbutton_set_dual_font(void *me, uv_font_st *font) {
	this->dualfont = font;
}

static inline uv_font_st *uv_uiduallabelbutton_get_dual_font(void *me, uv_font_st *font) {
	return this->dualfont;
}


/// @brief: Step function should be called every step cycle
static inline uv_uiobject_ret_e uv_uiduallabelbutton_step(void *me, uint16_t step_ms) {
	return uv_uibutton_step(me, step_ms);
}






#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIDUALLABELBUTTON_H_ */
