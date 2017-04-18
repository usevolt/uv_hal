/*
 * uwindow.c
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */


#include <ui/uv_uiwindow.h>
#include "ui/uv_uilabel.h"


#if CONFIG_LCD

#define this	((uv_uiwindow_st*) me)

#define SCROLLBAR_PADDING		5

static void draw_scrollbar(void *me, bool horizontal, const uv_bounding_box_st *pbb);

static void draw_scrollbar(void *me, bool horizontal, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;
	int16_t bar_x = (horizontal) ? (x + SCROLLBAR_PADDING) : (x + w - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH);
	int16_t bar_y = (horizontal) ? (y + h - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH) : (y + SCROLLBAR_PADDING);
	int16_t bar_w = (horizontal) ? (w - SCROLLBAR_PADDING * 2) : CONFIG_UI_WINDOW_SCROLLBAR_WIDTH;
	int16_t bar_h = (horizontal) ? CONFIG_UI_WINDOW_SCROLLBAR_WIDTH : (h - SCROLLBAR_PADDING * 2);



	// draw scroll bar background
	uv_lcd_draw_mrect(bar_x, bar_y, bar_w, bar_h,
			this->style->inactive_bg_c, pbb);

	float scale;
	uint16_t handle_h;
	uint16_t handle_w;
	float pos_scale;
	int16_t handle_y;
	int16_t handle_x;
	if (horizontal) {
		scale = (this->content_bb.width == 0) ? 1.0f : ((float) w / this->content_bb.width);
		handle_w = bar_w * scale;
		handle_h = CONFIG_UI_WINDOW_SCROLLBAR_WIDTH;
		pos_scale = - (float) this->content_bb.x /
				(this->content_bb.width - uv_uibb(this)->width);
		handle_x = bar_x + ((bar_w - handle_w) * pos_scale);
		handle_y = bar_y;
	}
	else {
		scale = (this->content_bb.height == 0) ? 1.0f : ((float) h / this->content_bb.height);
		handle_w = CONFIG_UI_WINDOW_SCROLLBAR_WIDTH;
		handle_h = bar_h * scale;
		pos_scale = - (float) this->content_bb.y /
				(this->content_bb.height - uv_uibb(this)->height);
		handle_x = bar_x;
		handle_y = bar_y + ((bar_h - handle_h) * pos_scale);
	}

	// draw handle
	uv_lcd_draw_mrect(handle_x, handle_y,handle_w, handle_h,
			this->style->active_fg_c, pbb);
}


/// @brief: Redraws this window
void _uv_uiwindow_redraw(const void *me, const uv_bounding_box_st *pbb) {
	if ((this->content_bb.height > uv_uibb(this)->height) ||
			(this->content_bb.width > uv_uibb(this)->width) ||
			!this->transparent) {

		uv_lcd_draw_mrect(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this), uv_uibb(this)->width,
				uv_uibb(this)->height, this->style->window_c, pbb);
	}
	if (this->content_bb.height > uv_uibb(this)->height) {
		draw_scrollbar(this, false, pbb);
	}
	if (this->content_bb.width > uv_uibb(this)->width) {
		draw_scrollbar(this, true, pbb);
	}
}

void uv_uiwindow_init(void *me, uv_uiobject_st **object_array, const uv_uistyle_st *style) {
	uv_uiobject_init((uv_uiobject_st*) this);
	uv_bounding_box_init(&this->content_bb, 0, 0, 0, 0);
	this->objects = object_array;
	this->objects_count = 0;
	this->style = style;
	this->dragging = false;
	this->transparent = false;
	this->app_step_callb = NULL;
	this->vrtl_draw = &_uv_uiwindow_redraw;
	((uv_uiobject_st*) this)->step_callb = &uv_uiwindow_step;
}



void uv_uiwindow_add(void *me, void *object,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height) {

	uv_bounding_box_init(&((uv_uiobject_st*) object)->bb, x, y, width, height);
	((uv_uiobject_st*) object)->parent = this;
	uv_ui_refresh_parent(object);
	this->objects[this->objects_count++] = object;
	if (this->content_bb.width == 0) {
		this->content_bb.width = uv_uibb(this)->width;
	}
	if (this->content_bb.height == 0) {
		this->content_bb.height = uv_uibb(this)->height;
	}
}

uv_bounding_box_st uv_uiwindow_get_contentbb(const void *me) {
	// if bounding box dimensions are 0, it is not set
	// and it should be initializes with window dimensions
	if (this->content_bb.width == 0) {
		this->content_bb.width = uv_uibb(this)->width;
	}
	if (this->content_bb.height == 0) {
		this->content_bb.height = uv_uibb(this)->height;
	}

	uv_bounding_box_st bb = this->content_bb;
	if (this->content_bb.width > uv_uibb(this)->width) {
		bb.height -= CONFIG_UI_WINDOW_SCROLLBAR_WIDTH;
	}
	if (this->content_bb.height > uv_uibb(this)->height) {
		bb.width -= CONFIG_UI_WINDOW_SCROLLBAR_WIDTH;
	}

	return bb;
}

void uv_uiwindow_set_contentbb(void *me, const int16_t width_px, const int16_t height_px) {
	this->content_bb.width = width_px;
	this->content_bb.height = height_px;
	this->content_bb.x = 0;
	this->content_bb.y = 0;
	uv_ui_refresh(this);
}

void uv_uiwindow_content_move(const void *me, const int16_t dx, const int16_t dy) {
	if ((this->content_bb.x + dx) > 0) {
		this->content_bb.x = 0;
	}
	else if ((this->content_bb.x + dx) < -(this->content_bb.width - uv_uibb(this)->width)) {
		this->content_bb.x = -this->content_bb.width + uv_uibb(this)->width;
	}
	else {
		this->content_bb.x += dx;
	}

	if ((this->content_bb.y + dy) > 0) {
		this->content_bb.y = 0;
	}
	else if ((this->content_bb.y + dy) < -(this->content_bb.height - uv_uibb(this)->height)) {
		this->content_bb.y = -this->content_bb.height + uv_uibb(this)->height;
	}
	else {
		this->content_bb.y += dy;
	}
	if (dx || dy) {
		uv_ui_refresh(this);
	}
}

uv_uiobject_ret_e uv_uiwindow_step(void *me, uv_touch_st *touch,
		uint16_t step_ms, const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (((uv_uiobject_st*)this)->refresh) {
		// first redraw this window
		this->vrtl_draw(this, pbb);
		ret = UIOBJECT_RETURN_REFRESH;
		// then request redraw all children objects
		uint16_t i;
		for (i = 0; i < this->objects_count; i++) {
			uv_ui_refresh_parent(this->objects[i]);
		}
		this->super.refresh = false;
	}
	// call step functions for all children which are visible
	uint16_t i;

	for (i = 0; i < this->objects_count; i++) {
		if (this->objects[i]->visible) {

			// touch event is unique for each children object
			uv_touch_st t2 = *touch;

			if (t2.action != TOUCH_NONE) {
				if (t2.action != TOUCH_DRAG) {
					t2.x -= uv_uibb(this->objects[i])->x + this->content_bb.x;
					t2.y -= uv_uibb(this->objects[i])->y + this->content_bb.y;
					if (t2.x > uv_uibb(this->objects[i])->width ||
							t2.y > uv_uibb(this->objects[i])->height ||
							t2.x < 0 ||
							t2.y < 0) {
						t2.action = TOUCH_NONE;
					}
				}
			}
			uv_touch_action_e touch_propagate = t2.action;

			// call child object's step callback
			uv_bounding_box_st bb = *uv_uibb(this);
			bb.x = uv_ui_get_xglobal(this);
			bb.y = uv_ui_get_yglobal(this);
			if (this->objects[i]->step_callb) {
				ret |= this->objects[i]->step_callb(this->objects[i], &t2, step_ms, &bb);
			}
			if (ret == UIOBJECT_RETURN_KILLED) {
				break;
			}

			// check if the object changed the touch event.
			// This means that the touch event is processed and it shouldn't
			// be propagating to other objects.
			if (t2.action != touch_propagate) {
				touch->action = TOUCH_NONE;
			}

		}
	}

	if (ret != UIOBJECT_RETURN_KILLED) {
		// if touch events were still pending, check if scroll bars have been clicked
		if ((this->content_bb.width > uv_uibb(this)->width) ||
				(this->content_bb.height > uv_uibb(this)->height)) {
			if (touch->action == TOUCH_IS_DOWN) {
				if (touch->x > (uv_uibb(this)->width - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH)) {
					if (touch->y < CONFIG_UI_WINDOW_SCROLLBAR_WIDTH) {
						// up pressed
						uv_uiwindow_content_move(this, 0, 5);
					}
					else if (touch->y > (uv_uibb(this)->height - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH)) {
						// down pressed
						uv_uiwindow_content_move(this, 0, -5);
					}
					else { }
				}
				if (touch->y > (uv_uibb(this)->height - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH)) {
					if (touch->x < CONFIG_UI_WINDOW_SCROLLBAR_WIDTH) {
						// left pressed
						uv_uiwindow_content_move(this, 5, 0);
					}
					else if (touch->x > (uv_uibb(this)->width - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH)) {
						// right pressed
						uv_uiwindow_content_move(this, -5, 0);
					}
					else { }
				}
			}
			else if (touch->action == TOUCH_PRESSED) {
				this->dragging = true;
				touch->action = TOUCH_NONE;
			}
			else if (touch->action == TOUCH_RELEASED) {
				this->dragging = false;
				touch->action = TOUCH_NONE;
			}
			else if (touch->action == TOUCH_DRAG) {
				if (this->dragging) {
					uv_uiwindow_content_move(this, touch->x, touch->y);
					touch->action = TOUCH_NONE;
				}
			}
			else {

			}
		}

		// lastly call application step callback if one is assigned
		if (this->app_step_callb != NULL) {
			ret |= this->app_step_callb(step_ms);
		}
	}


	return ret;
}


void uv_uiwindow_clear(void *me) {
	if (this->objects_count) {
		uv_ui_refresh(me);
	}
	this->objects_count = 0;
	this->app_step_callb = NULL;
}

void uv_uiwindow_set_transparent(void *me, bool value) {
	if (this->transparent != value) {
		uv_ui_refresh(this);
	}
	this->transparent = value;
}


#endif
