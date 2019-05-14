/*
 * uv_uilist.h
 *
 *  Created on: Oct 24, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UILIST_H_
#define UV_HAL_INC_UI_UV_UILIST_H_


#include <uv_hal_config.h>
#include <uv_utilities.h>
#include <uv_ui.h>

#if CONFIG_UI

#if !CONFIG_UI_LIST_ENTRY_HEIGHT
#error "CONFIG_UI_LIST_ENTRY_HEIGHT should define the height of a single entry in list in pixels"
#endif


/// @brief: List is a selectable vertical list of label entries
typedef struct {
	EXTENDS(uv_uiobject_st);

	/// @brief: List of the entry strings (char *)
	uv_vector_st entries;
	/// @brief: Index of the currently selected index
	int16_t selected_index;
	bool clicked;
	uint8_t clicked_index;
	color_t bg_c;
	color_t text_c;
	uv_font_st *font;

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
/// @param style: The UI style attached to this list
void uv_uilist_init(void *me, char **buffer, uint16_t buffer_len, const uv_uistyle_st *style);


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
uv_uiobject_ret_e uv_uilist_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb);


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
static inline void uv_uilist_select(void *me, int16_t index) {
	this->selected_index = index;
}


/// @brief: Pushes a new element into the end of the list
void uv_uilist_push_back(void *me, char *str);


/// @brief: Pops the last element from the list
void uv_uilist_pop_back(void *me, char *dest);

void uv_uilist_clear(void *me);


/// @brief: Indexes the list and returns the string at *index*
static inline char *uv_uilist_at(void *me, uint16_t index) {
	return (*((char **)uv_vector_at(&this->entries, index)));
}


/// @brief: Removes a *index*'th entry from the list
void uv_uilist_remove(void *me, uint16_t index);


void uv_uilist_insert(void *me, uint16_t index, char *str);


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UILIST_H_ */
