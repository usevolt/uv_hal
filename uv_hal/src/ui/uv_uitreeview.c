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


static uv_uiobject_ret_e _uv_uitreeobject_step(void *me, uv_touch_st *touch, uint16_t step_ms,
		const uv_bounding_box_st *pbb);
static void uv_uitreeobject_draw(const void *me, const uv_bounding_box_st *pbb);


void uv_uitreeobject_init(void *me, uv_uiobject_st **object_array,
		const char *name, void (*show_callb)(void), const uv_uistyle_st* style) {
	this->name = name;
	this->show_callb = show_callb;
	this->active = false;
	((uv_uiobject_st*) this)->step_callb = &_uv_uitreeobject_step;
	((uv_uiwindow_st*) this)->vrtl_draw = &uv_uitreeobject_draw;
	uv_uiwindow_init(this, object_array, style);
}


static void uv_uitreeobject_draw(const void *me, const uv_bounding_box_st *pbb) {

}


static uv_uiobject_ret_e _uv_uitreeobject_step(void *me, uv_touch_st *touch, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {

	return UIOBJECT_RETURN_ALIVE;
}


#undef this
#define this ((uv_uitreeview_st*) me)


static void draw(const void *me, const uv_bounding_box_st *pbb);



static void draw(const void *me, const uv_bounding_box_st *pbb) {
	// set treeview's content bounding box dimensions
	uv_ui_get_bb(this)->width -= XOFFSET;
	uv_ui_get_bb(this)->x = XOFFSET;
	uv_ui_get_bb(this)->height = height;
	printf("bb height: %u, content height: %u\n", uv_uibb(this)->height, uv_uiwindow_get_contentbb(this).height);


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

		uv_touch_st t = *touch;
		if (t.action != TOUCH_DRAG) {
			// substract content position from touch coordinates
			t.x -= this->super.content_bb.x;
			t.y -= this->super.content_bb.y;
		}

		if (this->object_array[i]->active) {

			uv_bounding_box_st bb = *uv_uibb(this->object_array[i]->window);
			bb.x = uv_ui_get_xglobal(this);
			bb.y = uv_ui_get_yglobal(this);
			if ((bb.x + bb.width) > (pbb->x + pbb->width)) {
				bb.width -= (bb.x + bb.width) - (pbb->x + pbb->width);
			}
			if ((bb.y + bb.height) > (pbb->y + pbb->height)) {
				bb.height -= (bb.y + bb.height) - (pbb->y + pbb->width);
			}

			// call active object's step function only if the object is active
			ret |= ((uv_uiobject_st*) this->object_array[i]->window)->
					step_callb(this->object_array[i]->window, &t, step_ms, &bb);

			// copy touch action to other children's touches
			// This way prevent touch action propagation to other children
			touch->action = t.action;

			// terminate if requested
			if (ret & UIOBJECT_RETURN_KILLED) {
				break;
			}
		}

	}

	if (!(ret & UIOBJECT_RETURN_KILLED)) {
		ret = uv_uiwindow_step(this, touch, step_ms, pbb);
	}

	return ret;

}


void uv_uitreeview_set_active(void *me, uv_uitreeobject_st *obj) {
	if (this->one_active) {
		for (uint16_t i = 0; i < this->obj_count; i++) {
			this->object_array[i]->active = false;
		}
	}
	obj->active = true;
	obj->show_callb();
	uv_ui_refresh(this);
}


void uv_uitreeview_add(void *me, uv_uitreeobject_st *object) {
	this->object_array[this->obj_count] = object;
	uv_ui_refresh(this);

	this->obj_count++;
}


#endif
