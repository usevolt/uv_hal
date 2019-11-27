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


#include <ui/uv_uiwindow.h>
#include "ui/uv_uilabel.h"


#if CONFIG_UI

#define this	((uv_uiwindow_st*) me)

#define SCROLLBAR_PADDING		2

static void draw_scrollbar(void *me, bool horizontal, const uv_bounding_box_st *pbb);

static void draw_scrollbar(void *me, bool horizontal, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;
	int16_t bar_x = (horizontal) ? (x + SCROLLBAR_PADDING) :
			(x + w - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH - SCROLLBAR_PADDING);
	int16_t bar_y = (horizontal) ? (y + h - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH - SCROLLBAR_PADDING) :
			(y + SCROLLBAR_PADDING);
	int16_t bar_w = (horizontal) ? (w - SCROLLBAR_PADDING * 2) : CONFIG_UI_WINDOW_SCROLLBAR_WIDTH;
	int16_t bar_h = (horizontal) ? CONFIG_UI_WINDOW_SCROLLBAR_WIDTH : (h - SCROLLBAR_PADDING * 2);

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

	// draw background
	uv_ft81x_draw_rrect(bar_x, bar_y, bar_w, bar_h,
			CONFIG_UI_WINDOW_SCROLLBAR_WIDTH / 2, this->bg_c);

	// draw handle
	uv_ft81x_draw_rrect(handle_x, handle_y, handle_w, handle_h,
			CONFIG_UI_WINDOW_SCROLLBAR_WIDTH / 2, this->handle_c);
}


/// @brief: Redraws this window
void uv_uiwindow_draw(void *me, const uv_bounding_box_st *pbb) {
	uv_bounding_box_st bb = *uv_uibb(this);
	bb.x = uv_ui_get_xglobal(this);
	bb.y = uv_ui_get_yglobal(this);
	if (bb.x < pbb->x) {
		bb.width -= pbb->x - bb.x;
		bb.x = pbb->x;
	}
	if (bb.y < pbb->y) {
		bb.height -= pbb->y - bb.y;
		bb.y = pbb->y;
	}
	if ((bb.x + bb.width) > (pbb->x + pbb->width)) {
		bb.width -= (bb.x + bb.width) - (pbb->x + pbb->width);
	}
	if ((bb.y + bb.height) > (pbb->y + pbb->height)) {
		bb.height -= (bb.y + bb.height) - (pbb->y + pbb->height);
	}

	uv_ft81x_set_mask(bb.x, bb.y, bb.width, bb.height);
	if (!this->transparent) {
		if (((color_st*) &this->bg_c)->a == 0xFF) {
			uv_ft81x_clear(this->bg_c);
		}
		else {
			uv_ft81x_draw_rrect(bb.x, bb.y, bb.width, bb.height, 0, this->bg_c);
		}
	}

	if (this->content_bb.height > uv_uibb(this)->height) {
		draw_scrollbar(this, false, pbb);
	}
	if (this->content_bb.width > uv_uibb(this)->width) {
		draw_scrollbar(this, true, pbb);
	}
}

void _uv_uiwindow_draw_children(void *me, const uv_bounding_box_st *pbb) {

	uv_bounding_box_st bb = *uv_uibb(this);
	int16_t globx = uv_ui_get_xglobal(this);
	int16_t globy = uv_ui_get_yglobal(this);
	bb.x = globx;
	bb.y = globy;
	if (bb.x < pbb->x) {
		bb.x = pbb->x;
	}
	if (bb.y < pbb->y) {
		bb.y = pbb->y;
	}
	if ((bb.x + bb.width) > (pbb->x + pbb->width)) {
		bb.width -= (bb.x + bb.width) - (pbb->x + pbb->width);
	}
	if ((bb.y + bb.height) > (pbb->y + pbb->height)) {
		bb.height -= (bb.y + bb.height) - (pbb->y + pbb->height);
	}

	for (int16_t i = 0; i < this->objects_count; i++) {
		((uv_uiobject_st*) this->objects[i])->refresh = true;
		bool ret = _uv_uiobject_draw(this->objects[i], pbb);

		// ensure that scissors mask is not changed by child object
		if (ret) {
			uv_ft81x_set_mask(globx, globy,
					uv_uibb(this)->width, uv_uibb(this)->height);
		}
	}
}


static void _uv_uiwindow_draw(void *me, const uv_bounding_box_st *pbb) {
	uv_uiwindow_draw(this, pbb);
	_uv_uiwindow_draw_children(this, pbb);
}


void uv_uiwindow_init(void *me, uv_uiobject_st **const object_array, const uv_uistyle_st *style) {
	uv_uiobject_init((uv_uiobject_st*) this);
	uv_bounding_box_init(&this->content_bb, 0, 0, 0, 0);
	this->content_bb_xdef = 0;
	this->content_bb_ydef = 0;
	this->objects = object_array;
	this->objects_count = 0;
	this->bg_c = style->window_c;
	this->handle_c = style->bg_c;
	this->dragging = false;
#if CONFIG_LCD
	// on LCD module transparent is by default false since
	// only part of the screen is updated
	this->transparent = false;
#elif CONFIG_FT81X
	// on FT81x transparent is by default true since whole screen
	// is always updated
	this->transparent = true;
#endif
	this->app_step_callb = NULL;
	this->user_ptr = NULL;
	uv_uiobject_set_draw_callb(this, &_uv_uiwindow_draw);
	uv_uiobject_set_touch_callb(this, &_uv_uiwindow_touch);
	uv_uiobject_set_step_callb(this, &uv_uiwindow_step);
}



void uv_uiwindow_add(void *me, void *object, uv_bounding_box_st *bb) {

	uv_bounding_box_init(&((uv_uiobject_st*) object)->bb, bb->x, bb->y, bb->width, bb->height);
	((uv_uiobject_st*) object)->parent = this;
	uv_ui_refresh(object);
	this->objects[this->objects_count++] = object;
	if (this->content_bb.width == 0) {
		this->content_bb.width = uv_uibb(this)->width;
	}
	if (this->content_bb.height == 0) {
		this->content_bb.height = uv_uibb(this)->height;
	}
}


void uv_uiwindow_remove(void *me, void *object) {
	bool found = false;
	for (uint16_t i = 0; i < this->objects_count; i++) {
		// move all objects after the found object one slot higher
		if (found) {
			this->objects[i - 1] = this->objects[i];
		}
		if (((void*) this->objects[i]) == object) {
			found = true;
		}
	}
	// lastly reduce object count by 1
	if (found) {
		this->objects_count--;
		uv_ui_refresh(this);
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
	uv_uiwindow_content_move(this, 0, 0);
	uv_ui_refresh(this);
}

void uv_uiwindow_content_move(const void *me, const int16_t dx, const int16_t dy) {
	if ((this->content_bb.x + dx) > this->content_bb_xdef) {
		this->content_bb.x = this->content_bb_xdef;
	}
	else if ((this->content_bb.x + dx) < -(this->content_bb.width - uv_uibb(this)->width)) {
		this->content_bb.x = -this->content_bb.width + uv_uibb(this)->width;
	}
	else {
		this->content_bb.x += dx;
	}

	if ((this->content_bb.y + dy) > this->content_bb_ydef) {
		this->content_bb.y = this->content_bb_ydef;
	}
	else if ((this->content_bb.y + dy) < -(this->content_bb.height - uv_uibb(this)->height)) {
		if (this->content_bb.height > uv_uibb(this)->height) {
			this->content_bb.y = -this->content_bb.height + uv_uibb(this)->height;
		}
		else {
			this->content_bb.y = this->content_bb_ydef;
		}
	}
	else {
		this->content_bb.y += dy;
	}
	if (dx || dy) {
		uv_ui_refresh(this);
	}
}




uv_uiobject_ret_e uv_uiwindow_step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	// call application step callback if one is assigned
	if ((this->app_step_callb != NULL) &&
			(((uv_uiobject_st*) this)->enabled)) {
		ret |= this->app_step_callb(this->user_ptr, step_ms);
	}

	if (!(ret & UIOBJECT_RETURN_KILLED)) {
		// call step functions for all children which are visible
		if (((uv_uiobject_st*) this)->visible) {
			for (int16_t i = 0; i < this->objects_count; i++) {
				if (this->objects[i]->visible) {

					// call child object's step function
					if (this->objects[i]->step_callb) {
						ret |= uv_uiobject_step(this->objects[i], step_ms);
					}
					if (ret & UIOBJECT_RETURN_KILLED) {
						break;
					}

				}
			}

		}
	}
	return ret;
}

void _uv_uiwindow_touch(void *me, uv_touch_st *touch) {
	// touch event is unique for each children object

	int16_t i;
	for (i = this->objects_count - 1; i >= 0; i--) {
		uv_touch_st t2 = *touch;

		if (this->objects[i]->visible &&
				this->objects[i]->enabled &&
				this->objects[i]->vrtl_touch) {
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

			// call child's touch callback
			this->objects[i]->vrtl_touch(this->objects[i], &t2);

			// check if the object changed the touch event.
			// This means that the touch event is processed and it shouldn't
			// be propagating to other objects.
			if (t2.action != touch_propagate) {
				touch->action = TOUCH_NONE;
			}
		}
	}
	// lastly handle own touch events
	// if touch events were still pending, check if scroll bars have been clicked or dragged
	if ((this->content_bb.width > uv_uibb(this)->width) ||
			(this->content_bb.height > uv_uibb(this)->height)) {
		if (touch->action == TOUCH_PRESSED) {
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
	// if window is not transparent, touches are always catched
	if (!this->transparent && (touch->action != TOUCH_DRAG)) {
		touch->action = TOUCH_NONE;
	}
}

void uv_uiwindow_set_stepcallback(void *me,
		uv_uiobject_ret_e (*step)(void *, const uint16_t),
		void *user_ptr) {
	this->app_step_callb = step;
	this->user_ptr = user_ptr;
}


void uv_uiwindow_set_content_bb_default_pos(void *me,
		const int16_t x, const int16_t y) {
	this->content_bb_xdef = x;
	this->content_bb_ydef = y;
	this->content_bb.x = x;
	this->content_bb.y = y;
}

void uv_uiwindow_clear(void *me) {
	if (this->objects_count) {
		uv_ui_refresh(me);
	}
	this->objects_count = 0;
	uv_uiwindow_set_stepcallback(me, NULL, NULL);
	uv_uiobject_set_draw_callb(me, &_uv_uiwindow_draw);
}

void uv_uiwindow_set_transparent(void *me, bool value) {
	if (this->transparent != value) {
		uv_ui_refresh(this);
	}
	this->transparent = value;
}


void uv_uiwindow_set_enabled(void *me, bool value) {
	for (uint16_t i = 0; i < this->objects_count; i++) {
		uv_uiobject_set_enabled(this->objects[i], value);
	}
	uv_uiobject_set_enabled(this, value);
}





#endif
