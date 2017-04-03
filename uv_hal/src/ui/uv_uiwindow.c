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

static void draw_scrollbar(void *me, bool horizontal);


static void draw_scrollbar(void *me, bool horizontal) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;
	int16_t bar_x = (horizontal) ? x : (x + w - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH);
	int16_t bar_y = (horizontal) ? (y + h - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH) : y;
	int16_t bar_w = (horizontal) ? w : CONFIG_UI_WINDOW_SCROLLBAR_WIDTH;
	int16_t bar_h = (horizontal) ? CONFIG_UI_WINDOW_SCROLLBAR_WIDTH : h;



	// draw scroll bar background
	uv_lcd_draw_mrect(bar_x, bar_y, bar_w, bar_h,
			this->style->inactive_bg_c, x, y, w, h);
	uv_lcd_draw_mframe(bar_x, bar_y, bar_w, bar_h, 1,
			this->style->inactive_frame_c, x, y, w, h);

	// draw upper button
	uv_lcd_draw_mrect(bar_x, bar_y, CONFIG_UI_WINDOW_SCROLLBAR_WIDTH,
			CONFIG_UI_WINDOW_SCROLLBAR_WIDTH, this->style->inactive_fg_c, x, y, w, h);
	uv_lcd_draw_mframe(bar_x, bar_y,CONFIG_UI_WINDOW_SCROLLBAR_WIDTH,
			CONFIG_UI_WINDOW_SCROLLBAR_WIDTH, 1, this->style->inactive_frame_c, x, y, w, h);
	_uv_ui_draw_text(bar_x + CONFIG_UI_WINDOW_SCROLLBAR_WIDTH / 2,
			bar_y + CONFIG_UI_WINDOW_SCROLLBAR_WIDTH / 2, this->style->font,
			ALIGN_CENTER, this->style->text_color, C(0xFFFFFFFF), (horizontal) ? "\x12" : "\x1f", 1.0f);

	// draw lower button
	uv_lcd_draw_mrect(bar_x + bar_w - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH,
			bar_y + bar_h - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH,
			CONFIG_UI_WINDOW_SCROLLBAR_WIDTH, CONFIG_UI_WINDOW_SCROLLBAR_WIDTH,
			this->style->inactive_fg_c, x, y, w, h);
	uv_lcd_draw_mframe(bar_x + bar_w - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH,
			bar_y + bar_h - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH,
			CONFIG_UI_WINDOW_SCROLLBAR_WIDTH, CONFIG_UI_WINDOW_SCROLLBAR_WIDTH,
			1, this->style->inactive_frame_c, x, y, w, h);
	_uv_ui_draw_text(bar_x + bar_w - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH / 2,
			bar_y + bar_h - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH / 2, this->style->font,
			ALIGN_CENTER, this->style->text_color, C(0xFFFFFFFF), (horizontal) ? "\x11" : "\x20", 1.0f);

	float scale;
	uint16_t handle_h;
	uint16_t handle_w;
	float pos_scale;
	int16_t handle_y;
	int16_t handle_x;
	if (horizontal) {
		scale = (this->content_bb.width == 0) ? 1.0f : ((float) w / this->content_bb.width);
		handle_w = uv_maxi(CONFIG_UI_WINDOW_SCROLLBAR_WIDTH,
				(w - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH * 2) * scale);
		handle_h = CONFIG_UI_WINDOW_SCROLLBAR_WIDTH;
		pos_scale = - (float) this->content_bb.x /
				(this->content_bb.width - uv_uibb(this)->width);
		handle_x = CONFIG_UI_WINDOW_SCROLLBAR_WIDTH +
				((x - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH * 2 - handle_w) * pos_scale);
		handle_y = bar_y;
	}
	else {
		scale = (this->content_bb.height == 0) ? 1.0f : ((float) h / this->content_bb.height);
		handle_w = CONFIG_UI_WINDOW_SCROLLBAR_WIDTH;
		handle_h = uv_maxi(CONFIG_UI_WINDOW_SCROLLBAR_WIDTH,
				(h - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH * 2) * scale);
		pos_scale = - (float) this->content_bb.y /
				(this->content_bb.height - uv_uibb(this)->height);
		handle_x = bar_x;
		handle_y = CONFIG_UI_WINDOW_SCROLLBAR_WIDTH +
				((y - CONFIG_UI_WINDOW_SCROLLBAR_WIDTH * 2 - handle_h) * pos_scale);
	}

	// draw handle
	uv_lcd_draw_mrect(handle_x, handle_y,handle_w, handle_h,
			this->style->inactive_fg_c, x, y, w, h);
	uv_lcd_draw_mframe(handle_x, handle_y, handle_w, handle_h, 1,
			this->style->inactive_frame_c, x, y, w, h);
}


/// @brief: Redraws this window
static void redraw(void *me) {

	uv_lcd_draw_rect(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_ui_get_bb(this)->width, uv_ui_get_bb(this)->height,
			this->style->window_c);

	if (this->content_bb.height > uv_uibb(this)->height) {
		draw_scrollbar(this, false);
	}
	if (this->content_bb.width > uv_uibb(this)->width) {
		draw_scrollbar(this, true);
	}
}

void uv_uiwindow_init(void *me,
		uv_uiobject_st **object_array, const uv_uistyle_st * style) {
	uv_uiobject_init((uv_uiobject_st*) this);
	uv_bounding_box_init(&this->content_bb, 0, 0,
			uv_uibb(this)->width, uv_uibb(this)->height);
	this->objects = object_array;
	this->objects_count = 0;
	this->style = style;
}


void uv_uiwindow_add(void *me, void *object,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height,
		void (*step_callb)(void*, uv_touch_st*, uint16_t)) {

	uv_bounding_box_init(&((uv_uiobject_st*) object)->bb, x, y, width, height);
	((uv_uiobject_st*) object)->parent = this;
	((uv_uiobject_st*) object)->step_callb = step_callb;
	((uv_uiobject_st*) object)->visible = true;
	((uv_uiobject_st*) object)->refresh = true;
	uv_ui_refresh_parent(object);
	this->objects[this->objects_count++] = object;
	if (this->content_bb.width == 0) {
		this->content_bb.width = uv_uibb(this)->width;
	}
	if (this->content_bb.height == 0) {
		this->content_bb.height = uv_uibb(this)->height;
	}
}


/// @brief: sets the content bounding box's width in pixels
void uv_uiwindow_set_contentbb_width(void *me, const int16_t width_px) {
	this->content_bb.width = width_px;
	if ((width_px > uv_uibb(this)->width) &&
			(this->content_bb.height == uv_uibb(this)->height)) {
		this->content_bb.height -= CONFIG_UI_WINDOW_SCROLLBAR_WIDTH;
	}
	uv_ui_refresh(this);
}

/// @brief: Sets the content bounding box's height in pixels
void uv_uiwindow_set_contentbb_height(void *me, const int16_t height_px) {
	this->content_bb.height = height_px;
	if ((height_px > uv_uibb(this)->height) &&
			(this->content_bb.width == uv_uibb(this)->width)) {
		this->content_bb.width -= CONFIG_UI_WINDOW_SCROLLBAR_WIDTH;
	}
	uv_ui_refresh(this);
}


void uv_uiwindow_step(void *me, uv_touch_st *touch, uint16_t step_ms) {

	if (((uv_uiobject_st*)this)->refresh) {
		// first redraw this window
		redraw(this);
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

			// touch event is unique for all children objects
			uv_touch_st t2 = *touch;
			if (t2.action != TOUCH_NONE) {
				if (t2.action != TOUCH_DRAG) {
					t2.x -= uv_uibb(this->objects[i])->x;
					t2.y -= uv_uibb(this->objects[i])->y;
					if (t2.x > uv_uibb(this->objects[i])->width ||
							t2.y > uv_uibb(this->objects[i])->height ||
							t2.x < 0 ||
							t2.y < 0) {
						t2.action = TOUCH_NONE;
					}
				}
			}
			uv_touch_action_e touch_propagate = t2.action;

			this->objects[i]->step_callb(this->objects[i], &t2, step_ms);
			// check if the object changed the touch event.
			// This means that the touch event is processed and it shouldn't
			// be propagating to other objects.
			if (t2.action != touch_propagate) {
				touch->action = TOUCH_NONE;
			}

		}
		// objects cannot set itself to refresh, so disable refresh request
		this->objects[i]->refresh = false;
	}
}

#endif
