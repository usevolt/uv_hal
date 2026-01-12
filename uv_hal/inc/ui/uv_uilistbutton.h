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

#ifndef UV_HAL_INC_UI_UV_UILISTBUTTON_H_
#define UV_HAL_INC_UI_UV_UILISTBUTTON_H_

#include <uv_hal_config.h>
#include "uv_ui.h"

#if CONFIG_UI


#if !defined(CONFIG_UI_LISTBUTTON_BAR_HEIGHT)
#error "CONFIG_UI_LISTBUTTON_BAR_HEIGHT should define the height used for listbutton content bars."
#endif


typedef enum {
	UILISTBUTTON_CONTENT_ARRAYOFPOINTER = 0,
	UILISTBUTTON_CONTENT_ARRAYOFSTRINGS
} uv_uilistbutton_content_e;

/// @brief: Button structure
typedef struct   {
	EXTENDS(uv_uibutton_st);
	char **content;
	char *title;
	uv_uilistbutton_content_e content_type;
	color_t activebar_c;
	color_t bar_c;
	uint8_t content_len;
	uint8_t current_index;
	uint16_t content_string_len;
} uv_uilistbutton_st;

#ifdef this
#undef this
#endif
#define this ((uv_uilistbutton_st*)me)

/// @brief: Initializes the button
///
/// @param content: a null-terminated string array holding the content for all list entries
/// @param content_len: The length of the content in entries
/// @param style: Pointer to the button style used
/// Pointer to this button object and the state of this button
void uv_uilistbutton_init(void *me, char **content,
		uint8_t content_len, uint8_t current_index, const uv_uistyle_st *style);


/// @brief: Sets the content type to array-of-pointers. This is the default behaviour.
/// The content buffer should be an array of pointers that point to the location of the strings.
static inline void uv_uilistbutton_set_content_type_arrayofpointers(void *me) {
	this->content_type = UILISTBUTTON_CONTENT_ARRAYOFPOINTER;
}

/// @brief: Sets the content type to array-of-strings. The content buffer
/// should be an array of constant length null-terminated strings. Good if
/// the content array is completely strored in RAM memory.
/// If custom length strings are needed, set this to 0.
static inline void uv_uilistbutton_set_content_type_arrayofstring(
		void *me, uint16_t string_lengths) {
	this->content_type = UILISTBUTTON_CONTENT_ARRAYOFSTRINGS;
	this->content_string_len = string_lengths;
}

/// @brief: Sets the title text. The title will be shown as a prefix to the actual content
/// on all entries. The title defaults to NULL.
static inline void uv_uilistbutton_set_title(void *me, char *value) {
	this->title = value;
}

/// @brief: Returns a pointer to the title text
static inline char *uv_uilistbutton_get_title(void *me) {
	return this->title;
}

/// @brief: Returns the current index
static inline uint8_t uv_uilistbutton_get_current_index(void *me) {
	return this->current_index;
}

static inline uint8_t uv_uilistbutton_get_index(void *me) {
	return uv_uilistbutton_get_current_index(me);
}

static inline uint8_t uv_uilistbutton_get_content_len(void *me) {
	return this->content_len;
}

/// @brief: Sets the current index. Should not be assigned bigger
/// (or equal) value than *content_len*
static inline void uv_uilistbutton_set_current_index(void *me, uint8_t value) {
	this->current_index = value;
}


/// @brief: sets the content buffer. Also need the content len parameter
static inline void uv_uilistbutton_set_content(void *me, char **buffer, uint8_t content_len) {
	this->content = buffer;
	this->content_len = content_len;
}

/// @brief: Returns true if the button was clicked
static inline bool uv_uilistbutton_clicked(void *me) {
	return uv_uibutton_clicked(me);
}

static inline bool uv_uilistbutton_is_down(void *me) {
	return uv_uibutton_is_down(me);
}


/// @brief: Returns true if the button was long pressed
static inline bool uv_uilistbutton_long_pressed(void *me) {
	return uv_uibutton_long_pressed(me);
}

/// @brief: Sets the main color of the uilistbutton. The button should be refreshed after
/// calling this.
static inline void uv_uilistbutton_set_main_color(void *me, color_t c) {
	uv_uibutton_set_main_color(me, c);
}

/// @brief: Returns the button main color
static inline color_t uv_uilistbutton_get_main_color(void *me) {
	return uv_uibutton_get_main_color(me);
}


/// @brief: Draw function. Normally this is called internally but it can also be
/// called when using draw callbacks
void uv_uilistbutton_draw(void *me, const uv_bounding_box_st *pbb);


#undef this
#define this ((uv_uimedialistbutton_st*)me)


/// @brief: uimedialistbutton is a listbutton that adds a media bitmap
typedef struct {
	EXTENDS(uv_uilistbutton_st);
	uv_uimedia_st *bitmap;
} uv_uimedialistbutton_st;

/// @brief: Initializes the uimedialistbutton
void uv_uimedialistbutton_init(void *me, char **content,
		uint8_t content_len, uint8_t current_index,
		uv_uimedia_st *bitmap,
		const uv_uistyle_st *style);


static inline uv_uimedia_st *uv_uimedialistbutton_get_media(void *me) {
	return this->bitmap;
}

void uv_uimedialistbutton_set_media(void *me, uv_uimedia_st *media);

static inline void uv_uimedialistbutton_set_content_type_arrayofpointers(void *me) {
	uv_uilistbutton_set_content_type_arrayofpointers(me);
}

static inline void uv_uimedialistbutton_set_content_type_arrayofstring(
		void *me, uint16_t string_lengths) {
	uv_uilistbutton_set_content_type_arrayofstring(me, string_lengths);
}

static inline void uv_uimedialistbutton_set_title(void *me, char *value) {
	uv_uilistbutton_set_title(me, value);
}

static inline char *uv_uimedialistbutton_get_title(void *me) {
	return uv_uilistbutton_get_title(me);
}

static inline uint8_t uv_uimedialistbutton_get_current_index(void *me) {
	return uv_uilistbutton_get_current_index(me);
}

static inline uint8_t uv_uimedialistbutton_get_index(void *me) {
	return uv_uilistbutton_get_index(me);
}

static inline uint8_t uv_uimedialistbutton_get_content_len(void *me) {
	return uv_uilistbutton_get_content_len(me);
}

static inline void uv_uimedialistbutton_set_current_index(void *me, uint8_t value) {
	uv_uilistbutton_set_current_index(me, value);
}


static inline void uv_uimedialistbutton_set_content(
		void *me, char **buffer, uint8_t content_len) {
	uv_uilistbutton_set_content(me, buffer, content_len);
}

static inline bool uv_uimedialistbutton_clicked(void *me) {
	return uv_uilistbutton_clicked(me);
}

static inline bool uv_uimedialistbutton_is_down(void *me) {
	return uv_uilistbutton_is_down(me);
}


static inline bool uv_uimedialistbutton_long_pressed(void *me) {
	return uv_uilistbutton_long_pressed(me);
}

static inline void uv_uimedialistbutton_set_main_color(void *me, color_t c) {
	uv_uilistbutton_set_main_color(me, c);
}

static inline color_t uv_uimedialistbutton_get_main_color(void *me) {
	return uv_uilistbutton_get_main_color(me);
}

void uv_uimedialistbutton_draw(void *me, const uv_bounding_box_st *pbb);


#endif

#endif /* UV_HAL_INC_UI_UV_UILISTBUTTON_H_ */
