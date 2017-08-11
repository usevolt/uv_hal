/*
 * uv_uiobject.c
 *
 *  Created on: Dec 23, 2016
 *      Author: usevolt
 */


#include "ui/uv_uiobject.h"

#if CONFIG_UI

#include "ui/uv_uiwindow.h"
#include "uv_utilities.h"
#include <stddef.h>
#include CONFIG_UI_DISPLAY_H

#define this ((uv_uiobject_st*) me)

/// @brief: Initializes the bounding box
void uv_bounding_box_init(uv_bounding_box_st *bb,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	bb->x = x;
	bb->y = y;
	bb->width = width;
	bb->height = height;
}



void uv_uiobject_init(void *me) {
	uv_bounding_box_init(&this->bb, 0, 0, 0, 0);
	this->parent = NULL;
	this->step_callb = NULL;
	this->visible = true;
	uv_ui_refresh(this);
	this->enabled = true;
	this->transition = NULL;
}


uv_uiobject_ret_e uv_uiobject_step(void *me, uv_touch_st *touch,
		uint16_t step_ms, const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;
	if (this->transition) {
		uv_uitransition_step(this->transition, this, step_ms);
	}
	if (this->step_callb) {
		ret = this->step_callb(this, touch, step_ms, pbb);
	}
	return ret;
}



void uv_ui_refresh_parent(void *me) {
#if CONFIG_LCD
	if (this->parent)  {
		((uv_uiobject_st*) this->parent)->refresh = true;
		// on FT81x the refresh request goes recursively all the way to
		// uidisplay. This way the whole display is updated when needed
		uv_ui_refresh_parent(this->parent);
	}
	this->refresh = true;
#elif CONFIG_FT81X
	uv_ui_refresh(me);
#endif
}


void uv_ui_set_enabled(void *me, bool enabled) {
	if (this->enabled != enabled) {
		if (this->enabled) {
			uv_ui_refresh_parent(this);
		}
		else {
			uv_ui_refresh(this);
		}
	}
	this->enabled = enabled;
}

/// @brief: Returns the X coordinate as global
///
/// @param this: Pointer to uv_uiobject_st casted to void*.
int16_t uv_ui_get_xglobal(const void *me) {
	int16_t x = this->bb.x;
	if (this->parent) {
		x += uv_ui_get_xglobal(this->parent);
		x += this->parent->content_bb.x;
	}
	return x;
}


/// @brief: Returns the Y coordinate as global
///
/// @param this: Pointer to uv_uiobject_st casted to void*.
int16_t uv_ui_get_yglobal(const void *me) {
	int16_t y = this->bb.y;
	if (this->parent) {
		y += uv_ui_get_yglobal(this->parent);
		y += this->parent->content_bb.y;
	}
	return y;
}



#if CONFIG_FT81X

void uv_ui_refresh(void *me) {
	((uv_uiobject_st*) me)->refresh = true;
	((uv_uiobject_st*) &CONFIG_UI_DISPLAY)->refresh = true;
}

#endif



#endif
