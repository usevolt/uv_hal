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


#ifndef UV_HAL_INC_UI_UV_UIFILEEDIT_H_
#define UV_HAL_INC_UI_UV_UIFILEEDIT_H_

#include "uv_utilities.h"
#include "uv_ui.h"


#if CONFIG_UI


/// @brief: A single file-type filter offered in the native file chooser.
typedef struct {
	/// @brief: Human-readable name shown in the chooser's filter selector,
	/// e.g. "Device files".
	const char *name;
	/// @brief: Space-separated glob patterns this filter matches,
	/// e.g. "*.json *.uvdev". Use "*" to match every file.
	const char *patterns;
} uv_uifileedit_filter_st;


/// @brief: File-selecting field. Drawn like uitextedit (a button-like
/// background with the selected file path rendered on top). When the path is
/// too long to fit, it is shortened with a leading "..." so the tail (file
/// name) stays visible.
///
/// On host targets (TARGET_LINUX, TARGET_WIN) a click pops up the operating
/// system's native file-chooser dialog; the chosen path is written back into
/// the caller-owned buffer. On MCU targets there is no host file system, so a
/// click does nothing.
typedef struct {
	EXTENDS(uv_uilabel_st);

	/// @brief: Caller-owned buffer holding the selected file path. The widget
	/// reads it for drawing and writes the chosen path into it.
	char *buffer;
	/// @brief: Size of *buffer* in bytes, including the null terminator.
	uint16_t buf_len;
	/// @brief: Optional title text drawn below the field, and also used as the
	/// native file-chooser window title. NULL = no title.
	char *title;
	color_t bg_color;
	bool changed;
	/// @brief: When true, a click opens a "save as" chooser (lets the user name a
	/// new file) instead of an "open" chooser. Defaults to false.
	bool save_mode;
	const uv_uistyle_st *style;
	/// @brief: Caller-owned array of selectable file-type filters, or NULL to
	/// allow all files. Must outlive the widget.
	const uv_uifileedit_filter_st *filters;
	uint8_t filter_count;
} uv_uifileedit_st;


#undef this
#define this ((uv_uifileedit_st*)me)


/// @brief: Opens the operating system's native file chooser directly (without a
/// fileedit widget) and writes the chosen path into *out*. *save* selects a
/// "save as" chooser that lets the user name a new file, over an "open existing"
/// chooser. Returns true if the user picked a file. Blocking; safe to call under
/// the FreeRTOS scheduler (preemption is disabled around the native dialog). On
/// MCU targets there is no host file system, so this returns false.
bool uv_uifiledialog_exec(const char *title,
		const uv_uifileedit_filter_st *filters, uint8_t filter_count,
		bool save, char *out, uint16_t out_len);


/// @brief: Initializes the fileedit. *buffer* must remain valid for the
/// lifetime of the widget. If *buffer* does not yet contain a null-terminated
/// string, it is initialized to a zero-length string.
void uv_uifileedit_init(void *me, char *buffer, uint16_t buf_len,
		const uv_uistyle_st *style);


/// @brief: Replaces the current path with *path*. Truncates to buf_len-1.
/// Does not set the *changed* flag (programmatic update, not a user choice).
void uv_uifileedit_set_path(void *me, const char *path);


/// @brief: Selects whether a click opens an "open file" chooser (false, the
/// default) or a "save as" chooser that lets the user type a new file name
/// (true).
static inline void uv_uifileedit_set_save_mode(void *me, bool value) {
	this->save_mode = value;
}


static inline const char *uv_uifileedit_get_path(void *me) {
	return this->buffer;
}


/// @brief: Returns true for one step cycle after the user picked a (different)
/// file from the native chooser.
static inline bool uv_uifileedit_value_changed(void *me) {
	return this->changed;
}


/// @brief: Restricts which files are selectable in the native chooser.
/// *filters* is a caller-owned array of *count* entries that must remain valid
/// for the lifetime of the widget. Pass count 0 (the default) to allow all files.
static inline void uv_uifileedit_set_filters(void *me,
		const uv_uifileedit_filter_st *filters, uint8_t count) {
	this->filters = filters;
	this->filter_count = count;
}


/// @brief: Sets the title text drawn below the field, also used as the native
/// file-chooser window title.
static inline void uv_uifileedit_set_title(void *me, char *value) {
	this->title = value;
}

static inline char *uv_uifileedit_get_title(void *me) {
	return this->title;
}


static inline void uv_uifileedit_set_bg_color(void *me, color_t c) {
	this->bg_color = c;
}


static inline void uv_uifileedit_set_text_color(void *me, color_t c) {
	uv_uilabel_set_color(me, c);
}


static inline void uv_uifileedit_set_font(void *me, uv_font_st *font) {
	uv_uilabel_set_font(me, font);
}

static inline uv_font_st *uv_uifileedit_get_font(void *me) {
	return uv_uilabel_get_font(me);
}


void uv_uifileedit_draw(void *me, const uv_bounding_box_st *pbb);


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIFILEEDIT_H_ */
