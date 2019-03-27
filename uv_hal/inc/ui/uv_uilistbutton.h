/*
 * uv_ubutton.h
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UILISTBUTTON_H_
#define UV_HAL_INC_UI_UV_UILISTBUTTON_H_

#include <uv_hal_config.h>
#include "uv_ui.h"

#if CONFIG_UI


#if !defined(CONFIG_UI_LISTBUTTON_BAR_HEIGHT)
#error "CONFIG_UI_LISTBUTTON_BAR_HEIGHT should define the heiht used for listbutton content bars."
#endif


/// @brief: Button structure
typedef struct {
	EXTENDS(uv_uibutton_st);
	char **content;
	char *title;
	color_t activebar_c;
	color_t bar_c;
	uint8_t content_len;
	uint8_t current_index;
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

/// @brief: Sets the current index. Should not be assigned bigger
/// (or equal) value than *content_len*
static inline void uv_uilistbutton_set_current_index(void *me, uint8_t value) {
	this->current_index = value;
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


/// @brief: Step function should be called every step cycle
uv_uiobject_ret_e uv_uilistbutton_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb);


/// @brief: Draw function. Normally this is called internally but it can also be
/// called when using draw callbacks
void uv_uilistbutton_draw(void *me, const uv_bounding_box_st *pbb);



#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UILISTBUTTON_H_ */