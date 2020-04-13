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

#ifndef UV_HAL_INC_UI_UV_UIMEDIALIST_H_
#define UV_HAL_INC_UI_UV_UIMEDIALIST_H_


#include <uv_hal_config.h>
#include <uv_utilities.h>
#include <uv_uilist.h>

#if CONFIG_UI



/// @brief: uimedialist_entry structure
typedef struct __attribute__((packed)) {
	EXTENDS(uv_uilist_entry_st);

	// the bitmap that is shown on this entry
	uv_uimedia_st *bitmap;
} uv_uimedialist_entry_st;


/// @brief: Initializes the uimedialist_entry with the given data
///
/// @param bitmap: Pointer to a uimedia structure that should already be initialized to
/// bitmap. If media should not be shown on this entry, set this to NULL.
static inline void uv_uimedialist_entry_init(uv_uimedialist_entry_st *this,
		char *text, uv_uimedia_st *bitmap) {
	uv_uilist_entry_init((uv_uilist_entry_st*) this, text);
	this->bitmap = bitmap;
}





/// @brief: List is a selectable vertical list of label entries
typedef struct __attribute__((packed)) {
	EXTENDS(uv_uilist_st);

} uv_uimedialist_st;



#ifdef this
#undef this
#endif
#define this ((uv_uimedialist_st*)me)



/// @brief: Initializes the media list
///
/// @param me: Pointer to the list structure
/// @param buffer: List of strings, e.g. (char*)-array
/// @param buffer_len: The maximum length of the buffer in elements
/// @param style: The UI style attached to this list
void uv_uimedialist_init(void *me, uv_uimedialist_entry_st *buffer,
		uint16_t buffer_len, const uv_uistyle_st *style);



/// @brief: Sets the vertical alignment of the medialist.
/// Uilist fills horizontally the space available.
/// Defaults to ALIGN_CENTER.
static inline void uv_uimedialist_set_align(void *me, alignment_e align) {
	uv_uilist_set_align(me, align);
}



/// @brief: Returns true for one step cycle if the list was clicked
static inline bool uv_uimedialist_clicked(void *me) {
	return uv_uilist_clicked(me);
}



/// @brief: Sets the background color of the uilist cells
static inline void uv_uimedialist_set_bgc(void *me, color_t c) {
	uv_uilist_set_bgc(me, c);
}



/// @brief: Returns the background color of the uilist
static inline color_t uv_uimedialist_get_bgc(void *me) {
	return uv_uilist_get_bgc(me);
}



/// @brief: Sets the text color of the uilist
static inline void uv_uimedialist_set_textc(void *me, color_t c) {
	uv_uilist_set_textc(me, c);
}



/// @brief: Gets the text color of the uilist
static inline color_t uv_uimedialist_get_textc(void *me) {
	return uv_uilist_get_textc(me);
}



/// @brief: Returns the font of the uilist
static inline uv_font_st *uv_uimedialist_get_font(void *me) {
	return uv_uilist_get_font(me);
}



/// @brief: Sets the font of the uilist
static inline void uv_uimedialist_set_font(void *me, uv_font_st *font) {
	uv_uilist_set_font(me, font);
}



/// @brief: Recalculates the height of this list and modifies the bounding box accordingly
static inline void uv_uimedialist_recalc_height(void *me) {
	uv_uilist_recalc_height(me);
}



/// @brief: Returns the selected entry's index. If none of the entries
/// is selected, returns -1.
static inline int16_t uv_uimedialist_get_selected(void *me) {
	return uv_uilist_get_selected(me);
}



/// @brief: Returns the current list entry count
static inline int16_t uv_uimedialist_get_count(void *me) {
	return uv_uilist_get_count(me);
}



/// @brief: Selects an entry from the list
static inline void uv_uimedialist_select(void *me, int16_t index) {
	uv_uilist_select(me, index);
}
static inline void uv_uimedialist_set_selected(void *me, int16_t index) {
	uv_uilist_set_selected(me, index);
}



/// @brief: Pushes a new element into the end of the list
static inline void uv_uimedialist_push_back(void *me, void *data) {
	uv_uilist_push_back(me, data);
}



/// @brief: Pops the last element from the list
static inline void uv_uimedialist_pop_back(void *me, void *dest) {
	uv_uilist_pop_back(me, dest);
}



static inline void uv_uimedialist_clear(void *me) {
	uv_uilist_clear(me);
}



/// @brief: Indexes the list and returns the string at *index*
static inline uv_uimedialist_entry_st *uv_uimedialist_at(void *me, uint16_t index) {
	return (uv_uimedialist_entry_st*) uv_uilist_at(me, index);
}



/// @brief: Removes a *index*'th entry from the list
void uv_uilist_remove(void *me, uint16_t index);



void uv_uilist_insert(void *me, uint16_t index, char *str);



#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIMEDIALIST_H_ */
