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

#ifndef UV_HAL_INC_UI_UV_UITOGGLEBUTTON_H_
#define UV_HAL_INC_UI_UV_UITOGGLEBUTTON_H_


#include <uv_hal_config.h>
#include "uv_ui.h"


#if CONFIG_UI

/// @brief: uitogglebutton is a button with a toggleable state.
typedef struct __attribute__((packed)) {
	EXTENDS(uv_uibutton_st);

	bool state;
	bool clicked;
} uv_uitogglebutton_st;

#ifdef this
#undef this
#endif
#define this ((uv_uitogglebutton_st*)me)


/// @brief: Initializes the togglebutton
///
/// @param state: The initial state of the button
void uv_uitogglebutton_init(void *me, bool state, char *text, const uv_uistyle_st *style);

static inline void uv_uitogglebutton_set_state(void *me, bool state) {
	if (this->state != state) {
		uv_ui_refresh(this);
	}
	this->state = state;
}


static inline bool uv_uitogglebutton_get_state(void *me) {
	return this->state;
}


static inline bool uv_uitogglebutton_clicked(void *me) {
	return this->clicked;
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
uv_uiobject_ret_e uv_uitogglebutton_step(void *me, uint16_t step_ms);



#undef this


#endif

#endif /* UV_HAL_INC_UI_UV_UITOGGLEBUTTON_H_ */
