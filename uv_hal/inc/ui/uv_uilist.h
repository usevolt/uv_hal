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

#if CONFIG_LCD

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
	/// @brief: UI style
	const uv_uistyle_st *style;
} uv_uilist_st;


#ifdef this
#undef this
#endif
#define this ((uv_uilist_st*)me)

/// @brief: Initializes the list
///
/// @param me: Pointer to the list structure
/// @param buffer: List of strings, e.g. (char*)-array
/// @param buffer_len: The length of the buffer in elements
/// @param style: The UI style attached to this list
static inline void uv_uilist_init(void *me, char **buffer,
		uint16_t buffer_len, const uv_uistyle_st *style) {
	uv_uiobject_init(me);
	this->selected_index = -1;
	uv_vector_init(&this->entries, buffer, buffer_len, sizeof(char*));
	this->style = style;
}


/// @brief: Step function is called every frame. It is also used to distinguish
/// different objects from each others in uv_uiwindow_st.
void uv_uilist_step(void *me, uv_touch_st *touch, uint16_t step_ms);


/// @brief: Recalculates the height of this list and modifies the bounding box accordingly
void uv_uilist_recalc_height(void *me);


/// @brief: Returns the selected entry's index. If none of the entries
/// is selected, returns -1.
static inline int16_t uv_uilist_get_selected(void *me) {
	return this->selected_index;
}

/// @brief: Selects an entry from the list
static inline void uv_uilist_select(void *me, int16_t index) {
	this->selected_index = index;
}


/// @brief: Pushes a new element into the end of the list
static inline void uv_uilist_push_back(void *me, char *str) {
	uv_vector_push_back(&this->entries, (void*) &str);
	uv_uilist_recalc_height(this);
	uv_ui_refresh_parent(this);
}

/// @brief: Pops the last element from the list
static inline void uv_uilist_pop_back(void *me, char *dest) {
	uv_vector_pop_back(&this->entries, &dest);
	uv_uilist_recalc_height(this);
	uv_ui_refresh_parent(this);
}

/// @brief: Indexes the list and returns the string at *index*
static inline char *uv_uilist_at(void *me, uint16_t index) {
	return (*((char **)uv_vector_at(&this->entries, index)));
}


/// @brief: Removes a *index*'th entry from the list
static inline void uv_uilist_remove(void *me, uint16_t index) {
	uv_vector_remove(&this->entries, index);
	uv_uilist_recalc_height(this);
	uv_ui_refresh_parent(this);
}

static inline void uv_uilist_insert(void *me, uint16_t index, char *str) {
	uv_vector_insert(&this->entries, index, &str);
	uv_uilist_recalc_height(this);
	uv_ui_refresh_parent(this);
}


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UILIST_H_ */
