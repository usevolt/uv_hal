/*
 * uv_ubutton.h
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UIBUTTON_H_
#define UV_HAL_INC_UI_UV_UIBUTTON_H_

#include <uv_hal_config.h>
#include "ui/uv_uiobject.h"
#include "ui/uv_uilabel.h"
#include "uv_utilities.h"

enum {
	BUTTON_UP = 0,
	BUTTON_CLICKED,
	BUTTON_LONGPRESSED
};
typedef uint8_t uibutton_state_e;



/// @brief: Button structure
typedef struct {
	EXTENDS(uv_uiobject_st);

	void (*callb)(void *, uibutton_state_e);
	uibutton_state_e state;
	char *text;
	const uv_uibutton_style_st *style;
} uv_uibutton_st;

#ifdef this
#undef this
#endif
#define this ((uv_uibutton_st*)me)

/// @brief: Initializes the button
///
/// @param text: The text which is displayed on the button
/// @param style: Pointer to the button style used
/// @param callb: Callback which will be called when the button is pressed. The parameters are:
/// Pointer to this button object and the state of this button
static inline void uv_uibutton_init(void *me, char *text, const uv_uibutton_style_st *style,
		void (*callb)(void *, uibutton_state_e)) {
	uv_uiobject_init(me);
	this->state = BUTTON_UP;
	this->style = style;
	this->text = text;
	this->callb = callb;
}


/// @brief: Sets the text of the button
static inline void uv_uibutton_set_text(void *me, char *text) {
	this->text = text;
	this->super.refresh = true;
}


/// @brief: Step function should be called every step cycle
void uv_uibutton_step(void *me, uv_touch_st *touch, uint16_t step_ms);



#undef this

#endif /* UV_HAL_INC_UI_UV_UIBUTTON_H_ */
