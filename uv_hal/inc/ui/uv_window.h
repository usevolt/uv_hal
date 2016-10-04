/*
 * uwindow.h
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_WINDOW_H_
#define UV_HAL_INC_UI_UV_WINDOW_H_



#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "ui/uv_object.h"
#include "uv_lcd.h"
#include "ui/uv_ui_styles.h"



#if CONFIG_LCD

/// @brief: A window GUI element. Window is an holder of other objects.
/// Inherits from the uv_ui_object_st.
typedef struct {
	EXTENDS(uv_ui_object_st);

	/// @brief: Array which holds the objects. The alignment of the objects is
	/// determined by the order which they reside in this array.
	/// The first index is the back-most object,
	/// and the last index is the top-most object on the display.
	uv_ui_object_st *objects;
	/// @brief: Object array length
	uint16_t objects_count;
	/// @brief: The GUI style attached to this window.
	/// Refer to uv_window_styles_st in uv_ui_styles.h for more info.
	uv_window_style_st *style;
} uv_window_st;




/// @brief: initializes the window
static inline void uv_window_init(uv_window_st *this,
		uv_ui_object_st *object_array, uv_window_style_st * style) {
	this->objects = object_array;
	this->objects_count = 0;
	this->style = style;
}



/// @brief: Window step function which takes care of the actions
/// of all objects. Should be called every step cycle.
///
/// @note: The step function also takes care of showing the window on the display.
/// Only those windows step functions should be called which are currently shown on the display.
void uv_window_step(void *me, uint16_t step_ms);




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
/// @param visible: True if the object should be visible
/// @param step_callb: Pointer to the appropriate object type's step function.
/// Note that this is the only thing which the window uses to distinguish different
/// object types from each other.
static inline void uv_window_add(uv_window_st *this, void *object,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height, bool visible,
		void (*step_callb)(void*, uint16_t)) {

	uv_ui_object_init(&this->objects[this->objects_count++], (uv_ui_object_st*) this,
			x, y, width, height, visible, step_callb);
}

#endif

#endif
