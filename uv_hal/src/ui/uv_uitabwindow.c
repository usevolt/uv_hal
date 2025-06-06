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


#include <string.h>
#include "ui/uv_uitabwindow.h"
#include "uv_uiwindow.h"

#if CONFIG_UI

#define this ((uv_uitabwindow_st*)me)

static void draw(void *me, const uv_bounding_box_st *pbb);
static void touch(void *me, uv_touch_st *touch);


char *get_tab_name(void *me, uint16_t tab_i) {
	char *ret = "";
	if (this->names_type == UITABWINDOW_LIST_OF_POINTERS) {
		ret = ((char**) this->tab_names)[tab_i];
	}
	else {
		// UITABWINDOW_LIST_OF_STRINGS
		// cycle through all the names until we get to the one requested
		ret = (char*) this->tab_names;
		for (uint8_t  i = 0; i < tab_i; i++) {
			ret += strlen(ret) + 1;
			// jump over additional termination characters if there are multiple ones
			while (*ret == '\0') {
				ret++;
			}
		}
	}
	return ret;
}


/// @param tab_names: Should be of type (char**) if the tab names are given as a list or pointers,
/// or (char*) if the tab names are given as a list of strings and also
/// uv_uitabwindow_set_tab_names_type has to be called and the type has to be specified to
/// UITABWINDOW_LIST_OF_STRINGS.
void uv_uitabwindow_init(void *me, int16_t tab_count,
		const uv_uistyle_st *style,
		uv_uiobject_st **obj_array,
		void *tab_names) {
	uv_uiwindow_init(this, obj_array, style);
	uv_uiwindow_set_transparent(this, false);
	uv_uiwindow_set_content_bb_default_pos(this, 0, CONFIG_UI_TABWINDOW_HEADER_HEIGHT);
	this->font = style->font;
	this->text_c = style->text_color;
	this->active_tab = 0;
	this->tab_count = tab_count;
	this->tab_names = tab_names;
	this->names_type = UITABWINDOW_LIST_OF_POINTERS;
	this->tab_changed = false;
	uv_uiobject_set_step_callb((uv_uiobject_st*) this, &uv_uitabwindow_step);
	uv_uiobject_set_draw_callb(this, &draw);
	uv_uiobject_set_touch_callb(this, &touch);
}


static void draw(void *me, const uv_bounding_box_st *pbb) {

	// super draw function
	uv_uiwindow_draw(this, pbb);

	int16_t thisx = uv_ui_get_xglobal(this);
	int16_t x = thisx;
	int16_t y = uv_ui_get_yglobal(this);
	int16_t tab_w = 0;
	int16_t active_tab_x = 0;
	int16_t active_tab_w = 0;


	for (int16_t i = 0; i < this->tab_count; i++) {
		tab_w = uv_ui_get_string_width(get_tab_name(this, i), this->font) + 10;
		if (tab_w < CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH) {
			tab_w = CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH;
		}
		if (this->active_tab != i) {
			uv_ui_draw_shadowrrect(x, y, tab_w, CONFIG_UI_TABWINDOW_HEADER_HEIGHT,
					CONFIG_UI_RADIUS, ((uv_uiwindow_st*) this)->bg_c,
					uv_uic_brighten(((uv_uiwindow_st*) this)->bg_c, 80),
					uv_uic_brighten(((uv_uiwindow_st*) this)->bg_c, -80));
			uv_ui_draw_string(get_tab_name(this, i), this->font,
					x + 4, y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT / 2, ALIGN_CENTER_LEFT,
					this->text_c);
		}
		else {
			active_tab_x = x;
			active_tab_w = tab_w;
		}
		x += tab_w;
	}
	// draw horizontal line
	uv_ui_draw_line(thisx, y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT - 1,
			thisx + uv_uibb(this)->width,
			y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT- 1, 1,
			C(0xFFFFFFFF));
	// draw active tab
	uv_ui_draw_shadowrrect(active_tab_x, y, active_tab_w, CONFIG_UI_TABWINDOW_HEADER_HEIGHT,
			CONFIG_UI_RADIUS, uv_uic_brighten(((uv_uiwindow_st*) this)->bg_c, 20),
			C(0xFFFFFFFF),
			C(0xFFFFFFFF));
	uv_ui_draw_string(get_tab_name(this, this->active_tab),
			this->font, active_tab_x + 5,
			y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT / 2, ALIGN_CENTER_LEFT,
			this->text_c);


	_uv_uiwindow_draw_children(this, pbb);
}

uv_bounding_box_st uv_uitabwindow_get_contentbb(void *me) {
	uv_bounding_box_st bb = *uv_uibb(me);
	bb.y += CONFIG_UI_TABWINDOW_HEADER_HEIGHT;
	bb.height -= CONFIG_UI_TABWINDOW_HEADER_HEIGHT;
	return bb;
}



uv_uiobject_ret_e uv_uitabwindow_step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	ret = uv_uiwindow_step(this, step_ms);

	// When tab has been changed, this->tab_changed has to be true for 1 step cycle
	this->tab_changed = false;
	return ret;
}


static void touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		if (touch->y <= CONFIG_UI_TABWINDOW_HEADER_HEIGHT) {
			int16_t total_w = 0;
			for (int16_t i = 0; i < this->tab_count; i++) {
				int16_t tab_w =
						uv_ui_get_string_height(get_tab_name(this, i), this->font) + 10;
				if (tab_w < CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH) {
					tab_w = CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH;
				}
				if (touch->x < total_w + tab_w) {
					this->active_tab = i;
					this->tab_changed = true;
					uv_ui_refresh(this);
					// prevent touch action from propagating further
					touch->action = TOUCH_NONE;
					break;
				}
				total_w += tab_w;
			}
		}
	}
	// let uv_uiwindow to handle propagating the touch event to all children
	_uv_uiwindow_touch(this, touch);
}


void uv_uitabwindow_clear(void *me) {
	void (*draw)(void *, const uv_bounding_box_st *) = ((uv_uiobject_st*) this)->vrtl_draw;
	uv_uiwindow_clear(this);
	uv_uiobject_set_draw_callb(this, draw);
}


#endif
