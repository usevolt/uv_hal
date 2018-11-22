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

#if CONFIG_UI


#if CONFIG_FT81X
#if !defined(CONFIG_UI_RADIUS)
#error "CONFIG_UI_RADIUS hsould define the radius of various UI elements on the screen"
#endif
#endif
#if !defined(CONFIG_UI_BUTTON_LONGPRESS_DELAY_MS)
#error "CONFIG_UI_BUTTON_LONGPRESS_DELAY_MS should define the delay in milliseconds which\
 results in a long press action on a uibutton."
#endif


enum {
	UIBUTTON_UP = 0,
	UIBUTTON_PRESSED,
	UIBUTTON_CLICKED,
	UIBUTTON_LONGPRESSED
};
typedef uint8_t uibutton_state_e;



/// @brief: Button structure
typedef struct {
	EXTENDS(uv_uiobject_st);

	uibutton_state_e state;
	uv_delay_st delay;
	char *text;
	color_t main_c;
	color_t text_c;
	const uv_font_st *font;

} uv_uibutton_st;

#ifdef this
#undef this
#endif
#define this ((uv_uibutton_st*)me)

/// @brief: Initializes the button
///
/// @param text: The text which is displayed on the button
/// @param style: Pointer to the button style used
/// Pointer to this button object and the state of this button
void uv_uibutton_init(void *me, char *text, const uv_uistyle_st *style);


/// @brief: Sets the text of the button
void uv_uibutton_set_text(void *me, char *text);

/// @brief: Returns the button text
static inline char *uv_uibutton_get_text(void *me) {
	return this->text;
}


/// @brief: Returns true if the button was clicked
static inline bool uv_uibutton_clicked(void *me) {
	return this->state == UIBUTTON_CLICKED;
}

static inline bool uv_uibutton_is_down(void *me) {
	return this->state == UIBUTTON_PRESSED;
}

/// @brief: Returns true if the button was long pressed
static inline bool uv_uibutton_long_pressed(void *me) {
	return this->state == UIBUTTON_LONGPRESSED;
}

/// @brief: Sets the main color of the uibutton. The button should be refreshed after
/// calling this.
static inline void uv_uibutton_set_main_color(void *me, color_t c) {
	this->main_c = c;
}


/// @brief: Step function should be called every step cycle
uv_uiobject_ret_e uv_uibutton_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb);



#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIBUTTON_H_ */
