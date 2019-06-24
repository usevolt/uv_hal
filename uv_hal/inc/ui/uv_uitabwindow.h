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

#ifndef UV_HAL_INC_UI_UV_UITABWINDOW_H_
#define UV_HAL_INC_UI_UV_UITABWINDOW_H_


#include <uv_hal_config.h>
#include <uv_ui.h>
#include <uv_lcd.h>


#if CONFIG_UI


#if !CONFIG_UI_TABWINDOW_HEADER_HEIGHT
#error "CONFIG_UI_TABWINDOW_HEADER_HEIGHT should define the tabwindow header height in pixels"
#endif
#if !CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH
#error "CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH should define the tabwindow header minimum width in pixels"
#endif


/// @brief: tab window is a window with different tabs
typedef struct {
	EXTENDS(uv_uiwindow_st);

	/// @brief: tells how many tabs this tabwindow has
	uint16_t tab_count;
	/// @brief: Pointers to the tab names
	const char **tab_names;
	/// @brief: Index of the current active tab
	uint16_t active_tab;
	/// @brief: True for 1 step cycle when the tab was changed
	bool tab_changed;

	color_t text_c;
	uv_font_st *font;
} uv_uitabwindow_st;

#ifndef this
#undef this
#endif
#define this ((uv_uitabwindow_st*)me)


/// @brief: Initializes the tab window
void uv_uitabwindow_init(void *me, int16_t tab_count,
		const uv_uistyle_st *style,
		uv_uiobject_st **obj_array,
		const char **tab_names);


static inline bool uv_uitabwindow_tab_changed(void *me) {
	return this->tab_changed;
}


static inline void uv_uitabwindow_set_tab(void *me, int16_t tab_index) {
	uv_ui_refresh(this);
	if (tab_index < this->tab_count) this->active_tab = tab_index;
}


static inline int16_t uv_uitabwindow_tab(void *me) {
	return this->active_tab;
}

/// @brief: implementation of uv_uiwindow's add function
static inline void uv_uitabwindow_addxy(void *me, void *object,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	uv_uiwindow_addxy(me, object, x, y, width, height);
}

static inline void uv_uitabwindow_add(void *me, void *object,
		uv_bounding_box_st *bb) {
	uv_uiwindow_add(me, object, bb);
}

static inline void uv_uitabwindow_clear(void *me) {
	uv_uiwindow_clear(this);
}

static inline void uv_uitabwindow_set_stepcallb(void *me,
		uv_uiobject_ret_e (*step)(void *, const uint16_t), void *user_ptr) {
	uv_uiwindow_set_stepcallback(me, step, user_ptr);
}

/// @brief: Returns the bounding box of the tab windows content
uv_bounding_box_st uv_uitabwindow_get_contentbb(void *me);

/// @brief: Step function is called from the owner window
uv_uiobject_ret_e uv_uitabwindow_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb);


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UITABWINDOW_H_ */
