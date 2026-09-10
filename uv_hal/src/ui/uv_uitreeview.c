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


#include "ui/uv_uitreeview.h"


#if CONFIG_UI

#define this ((uv_uitreeobject_st*) me)

#define XOFFSET	20


static void uitreeview_recalc_height(void *me);
static void uv_uitreeobject_draw(void *me, const uv_bounding_box_st *pbb);
static void touch(void *me, uv_touch_st *touch);



void uv_uitreeobject_init(void *me, uv_uiobject_st **object_array,
		const char *name, void (*show_callb)(uv_uitreeobject_st *me_ptr), const uv_uistyle_st* style) {
	uv_uiwindow_init(this, object_array, style);
	uv_uiwindow_set_content_bb_default_pos(this, 0, CONFIG_UI_TREEVIEW_ITEM_HEIGHT);
	this->text_c = style->text_color;
	this->font = style->font;
	this->name = name;
	this->show_callb = show_callb;
	this->open = false;
	((uv_uiobject_st*) this)->step_callb = &uv_uiwindow_step;
	uv_uiobject_set_draw_callb(this, &uv_uitreeobject_draw);
	uv_uiobject_set_touch_callb(this, &touch);
	uv_uiwindow_set_transparent(this, true);
}




static void touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		if ((touch->y >= 0) && (touch->y < CONFIG_UI_TREEVIEW_ITEM_HEIGHT)) {
			if (((uv_uiobject_st*) this)->parent != NULL) {
				if (this->open) {
					uv_uitreeview_close(((uv_uiobject_st*) this)->parent, this);
				}
				else {
					uv_uitreeview_open(((uv_uiobject_st*) this)->parent, this);
				}
			}
			touch->action = TOUCH_NONE;
		}
	}
}



void uv_uitreeobject_clear(void *me) {
	uv_uiwindow_clear(me);
	// uv_uiwindow_clear() ends by installing the plain window draw function,
	// which would cost this object its +/- marker, its name and its separator
	// line - and, since the children keep being drawn, leave the rows of an
	// open object floating with no header above them. Clearing the contents
	// does not stop it being a treeobject, so put the draw callback back.
	uv_uiobject_set_draw_callb(this, &uv_uitreeobject_draw);
}


static void uv_uitreeobject_draw(void *me, const uv_bounding_box_st *pbb) {

	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	// '+' means "opens", '-' means "closes": the marker shows what a click
	// would do, so a closed object carries the '+'.
	if (!this->open) {
		uv_ui_draw_string("+", this->font,
				x + XOFFSET, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2, ALIGN_CENTER_LEFT,
				this->text_c);
	}
	else {
		// super draw function
		uv_uiwindow_draw(this, pbb);

		uv_ui_draw_string("-", this->font,
				x + XOFFSET, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
				ALIGN_CENTER_LEFT, this->text_c);
	}

	uv_ui_draw_string((char*) this->name, this->font,
			x + XOFFSET * 2 +
			uv_ui_get_font_height(this->font),
			y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
			ALIGN_CENTER_LEFT, this->text_c);

	if (this->open) {
		y += uv_uibb(this)->height;
	}
	else {
		y += CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
	}
	uv_ui_draw_line(x, y - 1, x + w, y - 1, 1,
			uv_uic_brighten(((uv_uiwindow_st*) this)->bg_c, 30));

	// Only while open. Drawing them unconditionally meant closing an object
	// hid nothing: its rows stayed on screen, without the window background
	// that the open branch above paints, so closing merely made the contents
	// look dimmed instead of collapsing them.
	if (this->open) {
		_uv_uiwindow_draw_children(this, pbb);

		// scroll bars on top of the children
		uv_uiwindow_draw_scrollbars(this, pbb);
	}
}


uv_bounding_box_st uv_uitreeobject_get_content_bb(void *me) {
	uv_bounding_box_st bb = uv_uiwindow_get_contentbb(this);
	bb.height -= CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
	return bb;
}


#undef this
#define this ((uv_uitreeview_st*) me)




void uv_uitreeview_init(void *me,
		uv_uitreeobject_st ** const object_array, const uv_uistyle_st * style) {
	uv_uiwindow_init(this, (uv_uiobject_st ** const) object_array, style);
	this->one_active = true;
}




void uv_uitreeview_open(void *me, uv_uitreeobject_st *obj) {
	if (this->one_active) {
		for (uint16_t i = 0; i < ((uv_uiwindow_st*)this)->objects_count; i++) {
			if (((uv_uitreeobject_st*) ((uv_uiwindow_st*) this)->objects[i])->open) {
				uv_uitreeview_close(this,
						(uv_uitreeobject_st*) ((uv_uiwindow_st*)this)->objects[i]);
			}
		}
	}
	obj->open = true;
	// Back to the full height, so the rows below it are laid out (and can be
	// touched) again.
	uv_uibb(obj)->height = CONFIG_UI_TREEVIEW_ITEM_HEIGHT + obj->content_h;
	uitreeview_recalc_height(this);
	uv_uiwindow_content_move_to(this, 0, uv_uibb(obj)->y);
	if (obj->show_callb) {
		obj->show_callb(obj);
	}
}


void uv_uitreeview_close(void *me, uv_uitreeobject_st *obj) {
	obj->open = false;
	// Shrink to the header row. Without this a closed object keeps the height of
	// its open state, so it still covers the rows beneath it: its own hidden
	// children stay in the way of touches meant for the objects below.
	uv_uibb(obj)->height = CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
	uitreeview_recalc_height(this);
}


void uv_uitreeview_add(void *me, uv_uitreeobject_st * const object,
		const int16_t content_height, const bool active) {
	uv_uiwindow_addxy((uv_uiwindow_st*) this, object, 0,
			CONFIG_UI_TREEVIEW_ITEM_HEIGHT * ((uv_uiwindow_st*)this)->objects_count,
			uv_uiwindow_get_contentbb(this).width, CONFIG_UI_TREEVIEW_ITEM_HEIGHT + content_height);
	// Remembered so opening it again can restore the height that closing shrinks.
	object->content_h = content_height;
	if (active) {
		uv_uitreeview_open(this, object);
	}
	else {
		uv_uitreeview_close(this, object);
	}
}


static void uitreeview_recalc_height(void *me) {
	uint16_t content_height = 0;
	uv_uitreeobject_st ** const objs = (uv_uitreeobject_st ** const) ((uv_uiwindow_st*) this)->objects;
	for (int i = 0; i < ((uv_uiwindow_st*) this)->objects_count; i++) {
		uv_uibb(objs[i])->y = content_height;
		content_height += (objs[i]->open) ?
				uv_uibb(objs[i])->height : CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
	}
	uv_uiwindow_set_contentbb(this, uv_uibb(this)->width, content_height);
}



#endif


