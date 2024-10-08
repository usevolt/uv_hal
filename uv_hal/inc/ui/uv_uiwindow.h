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

#ifndef UV_HAL_INC_UI_UV_UIWINDOW_H_
#define UV_HAL_INC_UI_UV_UIWINDOW_H_



#include <ui/uv_uiobject.h>
#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_lcd.h"
#include "ui/uv_ui_styles.h"



#if CONFIG_UI


#if !defined(CONFIG_UI_WINDOW_SCROLLBAR_WIDTH)
#error "CONFIG_UI_WINDOW_SCROLLBAR_WIDTH should defined the width of the ui window scrollbar \
(height for horizontal scrollbar)"
#endif


/// @brief: A window GUI element. Window is an holder of other objects.
/// Inherits from the uv_uiobject_st.
///
/// @note: Type defined already in uv_uiobject.h
struct   _uv_uiwindow_st {
	EXTENDS(uv_uiobject_st);

	/// @brief: Children of this object reside in their own content bounding box.
	/// This bounding box is used for sliders
	uv_bounding_box_st content_bb;
	/// @brief: Content bounding boxes default x value. Set to 0 by uiwindow but
	/// child classes can change this (for example uitabview sets this to tab header height).
	int16_t content_bb_xdef;
	/// @brief: Content bounding boxes default y value
	int16_t content_bb_ydef;
	/// @brief: Array which holds the objects. The alignment of the objects is
	/// determined by the order which they reside in this array.
	/// The first index is the back-most object,
	/// and the last index is the top-most object on the display.
	uv_uiobject_st **objects;
	/// @brief: Object array length
	uint16_t objects_count;
	/// @brief: Indicates dragging has been started for this window
	bool dragging;
	/// @brief: Set to false if window should have a background. That is,
	/// if the window has any objects whose visibility will be toggled
	bool transparent;

	color_t bg_c;
	color_t handle_c;
	void *user_ptr;
	/// @brief: Application step callback. This will be called every step cycle
	/// after updating the UI. This should be used in the application to actually
	/// do anything with the UI.
	///
	/// @param ptr: A user defined pointer which can be used to hold a pointer to the actual
	/// UI module structure
	uv_uiobject_ret_e (*app_step_callb)(void *user_ptr, const uint16_t step_ms);
};




/// @brief: Window step function which takes care of the actions
/// of all objects. Should be called every step cycle.
///
/// @return: true if window or any of it's children updated the screen
///
/// @note: The step function also takes care of showing the window on the display.
/// Only those windows step functions should be called which are currently shown on the display.
uv_uiobject_ret_e uv_uiwindow_step(void *me, uint16_t step_ms);


#ifdef this
#undef this
#endif
#define this ((uv_uiwindow_st *)me)


/// @brief: initializes the window
void uv_uiwindow_init(void *me, uv_uiobject_st **const object_array, const uv_uistyle_st * style);


/// @brief: Returns the content bounding box
uv_bounding_box_st uv_uiwindow_get_contentbb(const void *me);

/// @brief: sets the content bounding box's width in pixels
void uv_uiwindow_set_contentbb(void *me, const int16_t width_px, const int16_t height_px);

/// @brief: Moves the content area *dx* and *dy* pixels in horizontal and vertical directions,
/// respectively.
void uv_uiwindow_content_move(const void *me, const int16_t dx, const int16_t dy);

/// @brief: Moves the content area to destination coordinates
static inline void uv_uiwindow_content_move_to(const void *me,
		const int16_t x, const int16_t y) {
	uv_uiwindow_content_move(this, -x - this->content_bb.x, -y - this->content_bb.y);
}

/// @brief: Registers an object to the window.
///
/// @note: Make sure that each object is registered ONLY ONCE and also that the
/// object array has enough room for the new object.
///
/// @param window: Pointer to the window where new object is added
/// @param object: Pointer to the object which is added. If the new object is button,
/// this should point to the button-structure.
/// @param bb: The bounding box structure defining the local coordinates and width and height
void uv_uiwindow_add(void *me, void *object, uv_bounding_box_st *bb);

/// @brief: Same as **uv_uiwindow_add* but with x, y, width and height instead of a boundign box
void uv_uiwindow_addxy(void *me, void *object,
		int16_t x, int16_t y, uint16_t width, uint16_t height);


/// @brief: Removes **object** from the window.
void uv_uiwindow_remove(void *me, void *object);


/// @brief: Sends the given **object** to the backmost in the drawing order.
///
/// @param object: The object that has to be this uiwindow's child added with *uv_uiwindow_add*.
void uv_uiwindow_send_to_back(void *me, void *object);


/// @brief: Adds a user application step callback function to this window.
/// The step function is called on every update step and should be used for
/// updating the display according to user input.
///
/// @param step: The step function pointer
/// @param user_ptr: Pointer for the application which can hold pointer to anything.
/// Useful for giving a UI structure pointer to the window
void uv_uiwindow_set_stepcallback(void *me,
		uv_uiobject_ret_e (*step)(void *user_ptr, const uint16_t step_ms), void *user_ptr);

/// @brief: Sets the content bounding boxes default position
void uv_uiwindow_set_content_bb_default_pos(void *me,
		const int16_t x, const int16_t y);

/// @brief: Clears of all objects added to the window, the application step callback
/// and also sets the draw callback to default value. This makes it more simple to
/// create different UI windows with the same objects.
void uv_uiwindow_clear(void *me);

/// @brief: If the window is not transparent, it's background will be drawn. By default
/// windows are set transparent to save performance.
void uv_uiwindow_set_transparent(void *me, bool value);

/// @brief: Returns the background color of this window
static inline color_t uv_uiwindow_get_bgc(void *me) {
	return this->bg_c;
}

/// @brief: Sets the background color for the window
static inline void uv_uiwindow_set_bgc(void *me, color_t c) {
	this->bg_c = c;
}


/// @brief: Enables or disabled the window. Compared to uv_uiobject_set_enabled,
/// This also affect all uiwindow's children
void uv_uiwindow_set_enabled(void *me, bool value);



/// @brief: Draws the background of the uiwindow
void uv_uiwindow_draw(void *me, const uv_bounding_box_st *pbb);


/// @brief: Draws the children objects. Will be called inside _uv_uiwindow_draw function,
/// but if custom draw function is added to this uiwindow, this should
/// be called at the end of that custom function to update the children.
void _uv_uiwindow_draw_children(void *me, const uv_bounding_box_st *pbb);

/// @brief: Internal uiwindow touch handler function. Can be used when inheriting uiwindow
/// in other modules
void _uv_uiwindow_touch(void *me, uv_touch_st *touch);


#undef this

#endif

#endif
