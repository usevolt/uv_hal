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

#ifndef UV_HAL_INC_UI_UV_UILIST_H_
#define UV_HAL_INC_UI_UV_UILIST_H_


#include <uv_hal_config.h>
#include <uv_utilities.h>
#include "uv_uiobject.h"

#if CONFIG_UI

#if !CONFIG_UI_LIST_ENTRY_HEIGHT
#error "CONFIG_UI_LIST_ENTRY_HEIGHT should define the height of a single entry in list in pixels"
#endif



typedef struct   {
	char *str;
} uv_uilist_entry_st;


/// @brief: Initializes the uilist with the given data
static inline void uv_uilist_entry_init(uv_uilist_entry_st *this, char *text) {
	this->str = text;
}



/// @brief: List is a selectable vertical list of label entries
typedef struct   {
	EXTENDS(uv_uiobject_st);

	/// @brief: List of the entries
	uv_vector_st entries;
	/// @brief: Index of the currently selected index
	int16_t selected_index;
	bool clicked;
	uint8_t clicked_index;
	color_t bg_c;
	color_t text_c;
	uv_font_st *font;
	// callback for drawing the entries
	void (*draw_entry)(void *me, void *entry, bool selected, uv_bounding_box_st *bb);
	alignment_e align;
} uv_uilist_st;



#ifdef this
#undef this
#endif
#define this ((uv_uilist_st*)me)



/// @brief: Initializes the list
///
/// @param me: Pointer to the list structure
/// @param buffer: List of strings, e.g. (char*)-array
/// @param buffer_len: The maximum length of the buffer in elements
/// @param entry_size: The size of the entry structure in bytes. For uilist modules this should
/// be *sizeof(uv_uilist_entry_st)*, something else for modules derived from this.
/// @param style: The UI style attached to this list
void uv_uilist_init(void *me, uv_uilist_entry_st *buffer,
		uint16_t buffer_len, const uv_uistyle_st *style);



/// @brief: Sets the vertical alignment of the list.
/// Uilist fills horizontally the space available.
/// Defaults to ALIGN_CENTER.
static inline void uv_uilist_set_align(void *me, alignment_e align) {
	this->align = align;
}



/// @brief: Returns true for one step cycle if the list was clicked
static inline bool uv_uilist_clicked(void *me) {
	return this->clicked;
}



/// @brief: Sets the element size for the entries. Defaults to *sizeof(uv_uilist_entry_st*),
/// but necessary to change for modules derived from uilist.
static inline void uv_uilist_set_entry_size(void *me, uint16_t element_size) {
	this->entries.element_size = element_size;
}



/// @brief: Sets the callback for drawing each entry on the uilist. Defaults to drawing
/// uv_uilist_entry_st. Should be set differently in modules derived from uilist.
static inline void uv_uilist_set_entry_draw(void *me,
		void (*draw_entry)(void *me, void *entry, bool selected, uv_bounding_box_st *bb)) {
	this->draw_entry = draw_entry;
}


/// @brief: Sets the background color of the uilist cells
static inline void uv_uilist_set_bgc(void *me, color_t c) {
	this->bg_c = c;
}



/// @brief: Returns the background color of the uilist
static inline color_t uv_uilist_get_bgc(void *me) {
	return this->bg_c;
}



/// @brief: Sets the text color of the uilist
static inline void uv_uilist_set_textc(void *me, color_t c) {
	this->text_c = c;
}



/// @brief: Gets the text color of the uilist
static inline color_t uv_uilist_get_textc(void *me) {
	return this->text_c;
}



/// @brief: Returns the font of the uilist
static inline uv_font_st *uv_uilist_get_font(void *me) {
	return this->font;
}



/// @brief: Sets the font of the uilist
static inline void uv_uilist_set_font(void *me, uv_font_st *font) {
	this->font = font;
}



/// @brief: Step function is called every frame. It is also used to distinguish
/// different objects from each others in uv_uiwindow_st.
uv_uiobject_ret_e uv_uilist_step(void *me, uint16_t step_ms);



/// @brief: Recalculates the height of this list and modifies the bounding box accordingly
void uv_uilist_recalc_height(void *me);



/// @brief: Returns the selected entry's index. If none of the entries
/// is selected, returns -1.
static inline int16_t uv_uilist_get_selected(void *me) {
	return this->selected_index;
}



/// @brief: Returns the current list entry count
static inline int16_t uv_uilist_get_count(void *me) {
	return uv_vector_size(&this->entries);
}



/// @brief: Selects an entry from the list
void uv_uilist_select(void *me, int16_t index);
static inline void uv_uilist_set_selected(void *me, int16_t index) {
	uv_uilist_select(me, index);
}



/// @brief: Pushes a new element into the end of the list
void uv_uilist_push_back(void *me, uv_uilist_entry_st *src);



/// @brief: Pops the last element from the list
void uv_uilist_pop_back(void *me, uv_uilist_entry_st *dest);



void uv_uilist_clear(void *me);



/// @brief: Indexes the list and returns the string at *index*
static inline uv_uilist_entry_st *uv_uilist_at(void *me, uint16_t index) {
	return (uv_uilist_entry_st*) uv_vector_at(&this->entries, index);
}



/// @brief: Removes a *index*'th entry from the list
void uv_uilist_remove(void *me, uint16_t index);



void uv_uilist_insert(void *me, uint16_t index, char *str);




#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UILIST_H_ */
