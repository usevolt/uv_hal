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


static void uitreeview_recalc_height(void *me);
static uv_uiobject_ret_e _uv_uitreeobject_step(void *me, uv_touch_st *touch, uint16_t step_ms,
		const uv_bounding_box_st *pbb);
static void uv_uitreeobject_draw(const void *me, const uv_bounding_box_st *pbb);
static void uv_uitreeobject_draw(const void *me, const uv_bounding_box_st *pbb);




void uv_uitreeobject_init(void *me, uv_uiobject_st **object_array,
		const char *name, void (*show_callb)(void), const uv_uistyle_st* style) {
	uv_uiwindow_init(this, object_array, style);
	this->name = name;
	this->show_callb = show_callb;
	((uv_uiobject_st*) this)->step_callb = &_uv_uitreeobject_step;
	((uv_uiwindow_st*) this)->vrtl_draw = &uv_uitreeobject_draw;
}



static uv_uiobject_ret_e _uv_uitreeobject_step(void *me, uv_touch_st *touch, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	ret = uv_uiwindow_step(this, touch, step_ms, pbb);


	return ret;
}



static void uv_uitreeobject_draw(const void *me, const uv_bounding_box_st *pbb) {

	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	if (!((uv_uiobject_st*) this)->enabled) {
		_uv_ui_draw_mtext(x + XOFFSET, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
				&CONFIG_UI_TREEVIEW_ARROW_FONT, ALIGN_CENTER_LEFT,
				((uv_uiwindow_st*) me)->style->active_bg_c,
				((uv_uiwindow_st*) me)->style->window_c, "\x10", 1.0f, pbb);
	}
	else {
		_uv_ui_draw_mtext(x + XOFFSET, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
				&CONFIG_UI_TREEVIEW_ARROW_FONT, ALIGN_CENTER_LEFT,
				((uv_uiwindow_st*) me)->style->active_bg_c,
				((uv_uiwindow_st*) me)->style->window_c, "\x1f", 1.0f, pbb);

		// super draw function
		_uv_uiwindow_redraw(this, pbb);
	}

	_uv_ui_draw_mtext(x + XOFFSET * 2 + CONFIG_UI_TREEVIEW_ARROW_FONT.char_width,
			y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
			((uv_uiwindow_st*) me)->style->font, ALIGN_CENTER_LEFT,
			((uv_uiwindow_st*) me)->style->text_color, C(0xFFFFFFFF),
			this->name, 1.0f, pbb);

	if (((uv_uiobject_st*) this)->enabled) {
		y += uv_uibb(this)->height +
				CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
	}
	else {
		y += CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
	}
	uv_lcd_draw_mrect(x, y, w, 1,
			((uv_uiwindow_st*) me)->style->inactive_frame_c, pbb);

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
			uv_uitreeview_close(this,
					(uv_uitreeobject_st*) ((uv_uiwindow_st*)this)->objects[i]);
		}
	}
	((uv_uiobject_st*) obj)->enabled = true;
	uitreeview_recalc_height(this);
	obj->show_callb();
}


void uv_uitreeview_close(void *me, uv_uitreeobject_st *obj) {
	((uv_uiobject_st*) obj)->enabled = false;
	uitreeview_recalc_height(this);
}


void uv_uitreeview_add(void *me, uv_uitreeobject_st * const object,
		const int16_t content_height, const bool active) {
	uv_uiwindow_add((uv_uiwindow_st*) this, object,
			XOFFSET, CONFIG_UI_TREEVIEW_ITEM_HEIGHT * ((uv_uiwindow_st*)this)->objects_count,
			uv_uiwindow_get_contentbb(this).width, CONFIG_UI_TREEVIEW_ITEM_HEIGHT + content_height);
	if (active) {
		uv_uitreeview_open(this, object);
	}
	else {
		uv_uitreeview_close(this, object);
	}
}


static void uitreeview_recalc_height(void *me) {
	uint16_t content_height = 0;
	for (int i = 0; i < ((uv_uiwindow_st*) this)->objects_count; i++) {
		uv_uitreeobject_st ** const objs = (uv_uitreeobject_st ** const) ((uv_uiwindow_st*) this)->objects;
		content_height += ((uv_uiobject_st*) this)->enabled ?
				uv_uibb(objs[i])->height : CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
	}
	uv_uiwindow_set_contentbb(this, uv_uibb(this)->width, content_height);
	uv_ui_refresh(this);
}



#endif


