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


#ifndef UV_HAL_INC_UI_UV_UITEXTEDIT_H_
#define UV_HAL_INC_UI_UV_UITEXTEDIT_H_

#include "uv_utilities.h"
#include "uv_ui.h"


#if CONFIG_UI


/// @brief: Cursor blink half-period in milliseconds (TARGET_LINUX in-place edit)
#define UITEXTEDIT_CURSOR_BLINK_MS	500


/// @brief: Bitmask flags controlling textedit behavior.
typedef enum {
	UITEXTEDIT_FLAG_NONE	= 0,
	/// @brief: Rejects newline characters so the buffer never contains '\n'.
	UITEXTEDIT_FLAG_ONELINE	= (1u << 0)
} uv_uitextedit_flags_e;


/// @brief: Editable text field. Behaves like uidigitedit but for strings:
/// on MCU targets a click opens the on-screen keyboard, on TARGET_LINUX
/// a click activates an in-place cursor and characters are read from the
/// host keyboard. Edit is committed by Enter or by clicking outside the field.
typedef struct {
	EXTENDS(uv_uilabel_st);

	/// @brief: Caller-owned text buffer. The widget reads/writes it directly.
	char *buffer;
	/// @brief: Size of *buffer* in bytes, including the null terminator.
	uint16_t buf_len;
	/// @brief: Optional title text drawn below the field. NULL = no title.
	char *title;
	/// @brief: Title shown on the on-screen keyboard popup (MCU targets).
	/// If NULL, *title* is used as a fallback.
	char *keyboard_title;
	color_t bg_color;
	bool changed;
	const uv_uistyle_st *style;
	/// @brief: Bitmask of UITEXTEDIT_FLAG_*.
	uv_uitextedit_flags_e flags;
#if CONFIG_TARGET_LINUX
	bool editing;
	bool was_touched;
	uint16_t blink_ms;
#endif
} uv_uitextedit_st;


#undef this
#define this ((uv_uitextedit_st*)me)


/// @brief: Initializes the textedit. *buffer* must remain valid for the
/// lifetime of the widget. If *buffer* does not yet contain a null-terminated
/// string, it is initialized to a zero-length string.
/// *flags* is a bitmask of UITEXTEDIT_FLAG_*; pass UITEXTEDIT_FLAG_NONE for defaults.
void uv_uitextedit_init(void *me, char *buffer, uint16_t buf_len,
		uv_uitextedit_flags_e flags, const uv_uistyle_st *style);


/// @brief: Replaces the current text with *text*. Truncates to buf_len-1.
/// Does not set the *changed* flag (programmatic update, not a user edit).
void uv_uitextedit_set_text(void *me, const char *text);


static inline const char *uv_uitextedit_get_text(void *me) {
	return this->buffer;
}


/// @brief: Returns true for one step cycle after the user committed an edit
/// (Enter pressed, on-screen keyboard accepted, or focus lost on Linux).
static inline bool uv_uitextedit_value_changed(void *me) {
	return this->changed;
}


/// @brief: Sets the title text drawn below the field.
static inline void uv_uitextedit_set_title(void *me, char *value) {
	this->title = value;
}

static inline char *uv_uitextedit_get_title(void *me) {
	return this->title;
}


/// @brief: Sets the title shown on the on-screen keyboard popup (MCU targets).
static inline void uv_uitextedit_set_keyboard_title(void *me, char *value) {
	this->keyboard_title = value;
}


static inline void uv_uitextedit_set_bg_color(void *me, color_t c) {
	this->bg_color = c;
}


static inline void uv_uitextedit_set_text_color(void *me, color_t c) {
	uv_uilabel_set_color(me, c);
}


static inline void uv_uitextedit_set_font(void *me, uv_font_st *font) {
	uv_uilabel_set_font(me, font);
}

static inline uv_font_st *uv_uitextedit_get_font(void *me) {
	return uv_uilabel_get_font(me);
}


void uv_uitextedit_draw(void *me, const uv_bounding_box_st *pbb);


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UITEXTEDIT_H_ */
