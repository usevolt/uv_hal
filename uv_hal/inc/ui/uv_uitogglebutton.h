/*
 * uv_uitogglebutton.h
 *
 *  Created on: Nov 1, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UITOGGLEBUTTON_H_
#define UV_HAL_INC_UI_UV_UITOGGLEBUTTON_H_


#include <uv_hal_config.h>
#include "uv_ui.h"


#if CONFIG_LCD

/// @brief: uitogglebutton is a button with a toggleable state.
typedef struct {
	EXTENDS(uv_uibutton_st);

	bool state;
} uv_uitogglebutton_st;

#ifdef this
#undef this
#endif
#define this ((uv_uitogglebutton_st*)me)


/// @brief: Initializes the togglebutton
///
/// @param state: The initial state of the button
static inline void uv_uitogglebutton_init(void *me, bool state, char *text, const uv_uistyle_st *style,
		void (*callb)(void *, uibutton_state_e)) {
	uv_uibutton_init(me, text, style, callb);
	this->state = state;
}

static inline void uv_uitogglebutton_set_state(void *me, bool state) {
	if (this->state != state) {
		uv_ui_refresh(this);
	}
	this->state = state;
}


static inline bool uv_uitogglebutton_get_state(void *me) {
	return this->state;
}


/// @brief: Sets the button text
static inline void uv_uitogglebutton_set_text(void *me, char *text) {
	uv_uibutton_set_text(me, text);
}

/// @brief: Returns the button text
static inline char *uv_uitogglebutton_get_text(void *me) {
	return uv_uibutton_get_text(me);
}


/// @brief: Step function should be called every step cycle
void uv_uitogglebutton_step(void *me, uv_touch_st *touch, uint16_t step_ms);



#undef this


#endif

#endif /* UV_HAL_INC_UI_UV_UITOGGLEBUTTON_H_ */
