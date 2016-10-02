/*
 * uv_uobject.h
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_OBJECT_H_
#define UV_HAL_INC_UI_UV_OBJECT_H_


#include <uv_hal_config.h>
#include "ui/uv_ui_styles.h"
#include "uv_utilities.h"
#include "uv_lcd.h"



/// @brief: Struct for individual object's bounding box.
typedef struct {
	/// @brief: local left-most GLOBAL x coordinate relative to the parent
	uint16_t x;
	/// @brief: Local top-most y-coordinate relative to the parent
	uint16_t y;
	/// @brief: Width, growing to right
	uint16_t width;
	/// @brief: Height, growing to bottom
	uint16_t height;
} uv_bounding_box_st;



/// @brief: Main struct for GUI object structure. Every GUI object should declare
/// this struct as it's first variable, e.g. they should inherit from this.
typedef struct uv_ui_object_st {
	/// @brief: Object bounding box
	uv_bounding_box_st bb;
	/// @brief: Pointer to the object's parent or NULL
	struct uv_ui_object_st *parent;
	/// @brief: Pointer to the object's step-function
	/// @param this: Pointer to this object
	/// @param step_ms: The step cycle duration in milliseconds
	/// @param touch: Touchscreen structure which holds the touchscreen actions made on this object
	void (*step_callb)(void *this, uint16_t step_ms);
	/// @brief: Object will be rendered on the screen only if this is set to true
	bool visible;
	/// @brief: A request to refresh this object. When this is set, the object will be completely
	/// redrawn on the next step cycle.
	/// @note: It depends on the object if this feature is implemented or not.
	bool refresh;
} uv_ui_object_st;



/// @brief: Initializes the bounding box
static inline void uv_bounding_box_init(uv_bounding_box_st *bb,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	bb->x = x;
	bb->y = y;
	bb->width = width;
	bb->height = height;
}



#define this ((uv_ui_object_st*) me)


/// @brief: Sets the refresh request to this object.
/// If the object has a parent, this requests refresh from the parent.
///
/// @param this: Pointer to uv_ui_object_st casted to void*.
static inline void uv_ui_refresh(void *me) {
	if (this->parent) this->parent->refresh = true;
	this->refresh = true;
}


/// @brief: Initializes an object
static inline void uv_ui_object_init(uv_ui_object_st *me, uv_ui_object_st *parent,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height, bool visible,
		void (*step_callb)(void*, uint16_t)) {

	uv_bounding_box_init(&this->bb, x, y, width, height);
	this->parent = parent;
	this->step_callb = step_callb;
	this->visible = visible;
	this->refresh = true;
	if (visible) uv_ui_refresh(this);
}


/// @brief: Hides the object form the display
///
/// @param this: Pointer to uv_ui_object_st casted to void*.
static inline void uv_ui_hide(void *me) {
	this->visible = false;
	uv_ui_refresh(this);
}


/// @brief: Shows the object
///
/// @param this: Pointer to uv_ui_object_st casted to void*.
static inline void uv_ui_show(void *me) {
	this->visible = true;
	uv_ui_refresh(this);
}

/// @brief: Getter for the object's bounding box
///
/// @param this: Pointer to uv_ui_object_st casted to void*.
static inline uv_bounding_box_st* uv_ui_get_bb(void *me) {
	return &this->bb;
}


/// @brief: Returns the X coordinate as global
///
/// @param this: Pointer to uv_ui_object_st casted to void*.
static inline uint16_t uv_ui_get_xglobal(void *me) {
	uint16_t x = this->bb.x;
	if (this->parent) {
		x += uv_ui_get_xglobal(this->parent);
	}
	return x;
}


/// @brief: Returns the Y coordinate as global
///
/// @param this: Pointer to uv_ui_object_st casted to void*.
static inline uint16_t uv_ui_get_yglobal(void *me) {
	uint16_t y = this->bb.y;
	if (this->parent) {
		y += uv_ui_get_yglobal(this->parent);
	}
	return y;
}

#undef this

#endif /* UV_HAL_INC_UI_UV_OBJECT_H_ */
