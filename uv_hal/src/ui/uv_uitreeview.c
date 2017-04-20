/*
 * uv_uitreewindow.c
 *
 *  Created on: Apr 12, 2017
 *      Author: usevolt
 */


#include "ui/uv_uitreeview.h"


#if CONFIG_LCD

#define this ((uv_uitreeobject_st*) me)

#define XOFFSET	5

void uv_uitreeobject_init(void *me, const char *name,
		uv_uiwindow_st *window, void (*show_callb)(void)) {
	this->name = name;
	this->window = window;
	this->show_callb = show_callb;
}


#undef this
#define this ((uv_uitreeview_st*) me)


static void draw(const void *me, const uv_bounding_box_st *pbb);





static void draw(const void *me, const uv_bounding_box_st *pbb) {

	// calculate the content height and objects' positions
	int16_t height = 0;
	for (uint16_t i = 0; i < this->obj_count; i++) {
		height += CONFIG_UI_TREEVIEW_ITEM_HEIGHT;

		uv_uibb(this->object_array[i]->window)->x = XOFFSET;
		uv_uibb(this->object_array[i]->window)->y = height;
		if (this->object_array[i]->active) {
			height += uv_uibb(this->object_array[i]->window)->height;
		}
	}
	((uv_uiwindow_st*) this)->content_bb.height = height;


	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;

	for (uint16_t i = 0; i < this->obj_count; i++) {

		if (!this->object_array[i]->active) {
			_uv_ui_draw_mtext(x + XOFFSET, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
					this->arrow_font, ALIGN_CENTER_LEFT,
					((uv_uiwindow_st*) me)->style->active_bg_c,
					((uv_uiwindow_st*) me)->style->window_c, "\x10", 1.0f, pbb);
		}
		else {
			_uv_ui_draw_mtext(x + XOFFSET, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
					this->arrow_font, ALIGN_CENTER_LEFT,
					((uv_uiwindow_st*) me)->style->active_bg_c,
					((uv_uiwindow_st*) me)->style->window_c, "\x1f", 1.0f, pbb);
		}

		_uv_ui_draw_mtext(x + XOFFSET * 2 + this->arrow_font->char_width,
				y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
				((uv_uiwindow_st*) me)->style->font, ALIGN_CENTER_LEFT,
				((uv_uiwindow_st*) me)->style->text_color, C(0xFFFFFFFF),
				this->object_array[i]->name, 1.0f, pbb);

		if (this->object_array[i]->active) {
			y += uv_uibb(this->object_array[i]->window)->height +
					CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
		}
		else {
			y += CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
		}
		uv_lcd_draw_mrect(x, y, w, 1,
				((uv_uiwindow_st*) me)->style->inactive_frame_c, pbb);

		if ((this->object_array[i]->active) &&
				(this->object_array[i]->show_callb != NULL) ) {
			// show up the active window
			printf("show\n");
			this->object_array[i]->show_callb();
		}
	}

}



void uv_uitreeview_init(void *me, uv_uitreeobject_st ** const object_array,
		const uv_font_st *arrow_font, const uv_uistyle_st * style) {
	uv_uiwindow_init(this, NULL, style);
	((uv_uiwindow_st*) me)->vrtl_draw = &draw;
	((uv_uiobject_st*) me)->step_callb = &_uv_uitreeview_step;
	this->object_array = object_array;
	this->arrow_font = arrow_font;
	this->obj_count = 0;
	this->one_active = true;
}



uv_uiobject_ret_e _uv_uitreeview_step(void *me, uv_touch_st *touch, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {

	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (touch->action != TOUCH_DRAG) {
		// convert touch to content space
		touch->y -= ((uv_uiwindow_st*) me)->content_bb.y;
	}

	for (uint16_t i = 0; i < this->obj_count; i++) {

		if (this->object_array[i]->active) {

		}
		else {

		}


		// call active object's step function
		uv_touch_st t = *touch;
		if (t.action != TOUCH_DRAG) {
			t.x -= uv_uibb(this->object_array[i]->window)->x;
			t.y -= uv_uibb(this->object_array[i]->window)->y;
		}
		uv_bounding_box_st bb = *uv_uibb(this->object_array[i]->window);
		bb.x = uv_ui_get_xglobal(me);
		bb.y = uv_ui_get_yglobal(me);
		if ((bb.x + bb.width) > (pbb->x + pbb->width)) {
			bb.width -= (bb.x + bb.width) - (pbb->x + pbb->width);
		}
		if ((bb.y + bb.height) > (pbb->y + pbb->height)) {
			bb.height -= (bb.y + bb.height) - (pbb->y + pbb->width);
		}
		ret |= ((uv_uiobject_st*) this->object_array[i]->window)->
				step_callb(this->object_array[i]->window, &t, step_ms, &bb);
		if (ret & UIOBJECT_RETURN_KILLED) {
			break;
		}

	}

	if (!(ret & UIOBJECT_RETURN_KILLED)) {
		ret = uv_uiwindow_step(this, touch, step_ms, pbb);
	}

	return ret;

}


void uv_uitreeview_set_active(void *me, uint16_t active_index) {
	if (active_index < this->obj_count) {
		if (this->one_active) {
			for (uint16_t i = 0; i < this->obj_count; i++) {
				this->object_array[i]->active = false;
			}
		}
		this->object_array[active_index]->active = true;
		uv_ui_refresh(this);
	}
}


void uv_uitreeview_add(void *me, uv_uitreeobject_st *object,
		int16_t width, int16_t height) {
	this->object_array[this->obj_count] = object;
	// x and y can be set to 0. They are recalculated in draw function,
	// depending on active objects.
	uv_bounding_box_init(&((uv_uiobject_st*) this->object_array[this->obj_count]->window)->bb,
			0, 0, width, height);
	((uv_uiobject_st*) object->window)->parent = me;
	uv_ui_refresh(this);

	this->obj_count++;
}


#endif
