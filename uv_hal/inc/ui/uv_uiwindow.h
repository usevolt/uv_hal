/*
 * uwindow.h
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
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
struct _uv_uiwindow_st {
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
	/// @brief: The GUI style attached to this window.
	/// Refer to uv_uiwindow_styles_st in uv_ui_styles.h for more info.
	const uv_uistyle_st *style;
	/// @brief: Virtual draw function. Window itself doesn't provide any drawable graphics,
	/// but structures extending from this one should implement this
	void (*vrtl_draw)(const void *me, const uv_bounding_box_st *pbb);
	/// @brief: Application step callback. This will be called every step cycle
	/// after updating the UI. This should be used in the application to actually
	/// do anything with the UI.
	uv_uiobject_ret_e (*app_step_callb)(const uint16_t step_ms);
};




/// @brief: Window step function which takes care of the actions
/// of all objects. Should be called every step cycle.
///
/// @return: true if window or any of it's children updated the screen
///
/// @note: The step function also takes care of showing the window on the display.
/// Only those windows step functions should be called which are currently shown on the display.
uv_uiobject_ret_e uv_uiwindow_step(void *me, uv_touch_st *touch,
		uint16_t step_ms, const uv_bounding_box_st *pbb);


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
/// @param x: The object's bounding box's left-most coordinate relative to the window
/// @param y: The object's bounding box's top-most coordinate relative to the window
/// @param width: The width of the object in pixels. Some objects might not need this since
/// they can calculate their own width. In this case give 0.
/// @param height: The height of the object in pixels. Some ojects might not need this since
/// they can calculate their own height, such as labels. In this case give 0.
/// @param visible: True if the object should be visible
void uv_uiwindow_add(void *me, void *object,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height);

static inline void uv_uiwindow_set_stepcallback(void *me,
		uv_uiobject_ret_e (*step)(const uint16_t step_ms)) {
	this->app_step_callb = step;
}

/// @brief: Sets the content bounding boxes default position
void uv_uiwindow_set_content_bb_default_pos(void *me,
		const int16_t x, const int16_t y);

/// @brief: Clears the object buffer memory clearing the whole window
void uv_uiwindow_clear(void *me);

/// @brief: If the window is not transparent, it's background will be drawn. By default
/// windows are set transparent to save performance.
void uv_uiwindow_set_transparent(void *me, bool value);


/// @brief: Redraw function for internal use
void _uv_uiwindow_redraw(const void *me, const uv_bounding_box_st *pbb);


#undef this

#endif

#endif
