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

#ifndef UV_HAL_INC_UI_UV_UITREEVIEW_H_
#define UV_HAL_INC_UI_UV_UITREEVIEW_H_


#include <uv_hal_config.h>
#include "uv_ui.h"
#include "uv_lcd.h"

#if CONFIG_UI


#if !defined(CONFIG_UI_TREEVIEW_ITEM_HEIGHT)
#error "CONFIG_UI_TREEVIEW_ITEM_HEIGHT should define the tree view object's header's height in pixels"
#endif
#if !defined(CONFIG_UI_TREEVIEW_ARROW_FONT)
#error "CONFIG_UI_TREEVIEW_ARROW_FONT should define the font used for treeobject's opening arrows"
#endif



typedef struct _uv_uitreeobject_st uv_uitreeobject_st;

/// @brief: Object structure which will be shown as a single treewindow object
struct   _uv_uitreeobject_st {
	EXTENDS(uv_uiwindow_st);
	/// @brief: Visible name of the object
	const char *name;
	/// @brief: Function callback for showing this object's window
	void (*show_callb)(uv_uitreeobject_st *me_ptr);

	color_t text_c;
	uv_font_st *font;
};


typedef struct {
	EXTENDS(uv_uiwindow_st);

	bool one_active;

} uv_uitreeview_st;





/// @brief: Initializes uitreeobject. This should be called for all uitreeviewobjects which
/// are not defined as constants.
void uv_uitreeobject_init(void *me, uv_uiobject_st **object_array,
		const char *name, void (*show_callb)(uv_uitreeobject_st *me_ptr), const uv_uistyle_st* style);


/// @brief: Adds objects to the uitreeobject. This should be called
/// in a uitreeiobject's show-callback
static inline void uv_uitreeobject_addxy(void *me, void* obj,
		int16_t x, int16_t y, uint16_t width, uint16_t height) {
	uv_uiwindow_addxy(me, obj, x, y, width, height);
}

static inline void uv_uitreeobject_add(void *me, void* obj,
		uv_bounding_box_st *bb) {
	uv_uiwindow_add(me, obj, bb);
}

uv_bounding_box_st uv_uitreeobject_get_content_bb(void *me);

static inline void uv_uitreeobject_set_content_bb(void *me,
		const int16_t width, const int16_t height) {
	uv_uiwindow_set_contentbb(me, width, height + CONFIG_UI_TREEVIEW_ITEM_HEIGHT);
}

static inline void uv_uitreeobject_clear(void *me) {
	uv_uiwindow_clear(me);
}
static inline void uv_uitreeobject_set_step_callback(void *me,
		uv_uiobject_ret_e (*step)(void *user_ptr, const uint16_t step_ms),
		void *user_ptr) {
	uv_uiwindow_set_stepcallback(me, user_ptr, step);
}




/// @brief: Initializes a treeview. A Treeview contains cascadeable objects
/// In a tree-kind of way.
///
/// @param object_array: Pointer to an array of child structures which contain a pointer
/// to the window structure and it's name.
void uv_uitreeview_init(void *me,
		uv_uitreeobject_st ** const object_array, const uv_uistyle_st * style);



/// @brief: Opens a treeobject
void uv_uitreeview_open(void *me, uv_uitreeobject_st *obj);

/// @brief: Closes a treeobject
void uv_uitreeview_close(void *me, uv_uitreeobject_st *obj);


/// @brief: By default only 1 object can be active (== open) at one time
static inline void uv_uitreeview_set_oneactive(void *me, bool value) {
	((uv_uitreeview_st*) me)->one_active = value;
	uv_ui_refresh(me);
}


static inline void uv_uitreeview_set_stepcallb(void *me,
		uv_uiobject_ret_e (*step)(void *, const uint16_t), void *user_ptr) {
	uv_uiwindow_set_stepcallback(me, step, user_ptr);
}



/// @brief: Adds a new object to the treeview
///
/// @param object: Pointer to the object to be added
void uv_uitreeview_add(void *me, uv_uitreeobject_st * const object,
		const int16_t content_height, const bool active);

#endif


#endif /* UV_HAL_INC_UI_UV_UITREEVIEW_H_ */
