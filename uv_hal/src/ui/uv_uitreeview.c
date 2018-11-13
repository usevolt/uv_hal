/*
 * uv_uitreewindow.c
 *
 *  Created on: Apr 12, 2017
 *      Author: usevolt
 */


#include "ui/uv_uitreeview.h"


#if CONFIG_UI

#define this ((uv_uitreeobject_st*) me)

#define XOFFSET	20


static void uitreeview_recalc_height(void *me);
static uv_uiobject_ret_e _uv_uitreeobject_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb);
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
	((uv_uiobject_st*) this)->step_callb = &_uv_uitreeobject_step;
	uv_uiobject_set_draw_callb(this, &uv_uitreeobject_draw);
	uv_uiobject_set_touch_callb(this, &touch);
	uv_uiwindow_set_transparent(this, true);
}



static uv_uiobject_ret_e _uv_uitreeobject_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	ret = uv_uiwindow_step(this, step_ms, pbb);

	return ret;
}

static void touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		if ((touch->y >= 0) && (touch->y < CONFIG_UI_TREEVIEW_ITEM_HEIGHT)) {
			if (((uv_uiobject_st *) this)->enabled) {
				uv_uitreeview_close(((uv_uiobject_st*) this)->parent, this);
			}
			else {
				uv_uitreeview_open(((uv_uiobject_st*) this)->parent, this);
			}
			touch->action = TOUCH_NONE;
		}
	}
}



static void uv_uitreeobject_draw(void *me, const uv_bounding_box_st *pbb) {

	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	if (!((uv_uiobject_st*) this)->enabled) {
		uv_ft81x_draw_string("-", this->font->index,
				x + XOFFSET, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2, ALIGN_CENTER_LEFT,
				this->text_c);
	}
	else {
		// super draw function
		_uv_uiwindow_redraw(this, pbb);

#if CONFIG_LCD
		_uv_ui_draw_mtext(x + XOFFSET, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
				&CONFIG_UI_TREEVIEW_ARROW_FONT, ALIGN_CENTER_LEFT,
				((uv_uiwindow_st*) me)->style->active_bg_c,
				((uv_uiwindow_st*) me)->style->window_c, "\x1f", 1.0f, pbb);
#elif CONFIG_FT81X
		uv_ft81x_draw_string("+", this->font->index,
				x + XOFFSET, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
				ALIGN_CENTER_LEFT, this->text_c);
#endif
	}

#if CONFIG_LCD
	_uv_ui_draw_mtext(x + XOFFSET * 2 + CONFIG_UI_TREEVIEW_ARROW_FONT.char_width,
			y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
			((uv_uiwindow_st*) me)->style->font, ALIGN_CENTER_LEFT,
			((uv_uiwindow_st*) me)->style->text_color, C(0xFFFFFFFF),
			this->name, 1.0f, pbb);
#elif CONFIG_FT81X
	uv_ft81x_draw_string((char*) this->name, this->font->index,
			x + XOFFSET * 2 +
			uv_ft81x_get_font_height(this->font->index),
			y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
			ALIGN_CENTER_LEFT, this->text_c);
#endif

	if (((uv_uiobject_st*) this)->enabled) {
		y += uv_uibb(this)->height;
	}
	else {
		y += CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
	}
#if CONFIG_LCD
	uv_lcd_draw_mrect(x, y - 1, w, 1,
			((uv_uiwindow_st*) me)->style->inactive_frame_c, pbb);
#elif CONFIG_FT81X
	uv_ft81x_draw_line(x, y - 1, x + w, y - 1, 1,
			uv_uic_brighten(((uv_uiwindow_st*) this)->bg_c, 30));
#endif
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
			if (((uv_uiobject_st*) ((uv_uiwindow_st*) this)->objects[i])->enabled) {
				uv_uitreeview_close(this,
						(uv_uitreeobject_st*) ((uv_uiwindow_st*)this)->objects[i]);
			}
		}
	}
	((uv_uiobject_st*) obj)->enabled = true;
	uitreeview_recalc_height(this);
	uv_uiwindow_content_move_to(this, 0, uv_uibb(obj)->y);
	if (obj->show_callb) {
		obj->show_callb(obj);
	}
}


void uv_uitreeview_close(void *me, uv_uitreeobject_st *obj) {
	((uv_uiobject_st*) obj)->enabled = false;
	uitreeview_recalc_height(this);
}


void uv_uitreeview_add(void *me, uv_uitreeobject_st * const object,
		const int16_t content_height, const bool active) {
	uv_uiwindow_add((uv_uiwindow_st*) this, object, 0,
			CONFIG_UI_TREEVIEW_ITEM_HEIGHT * ((uv_uiwindow_st*)this)->objects_count,
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
	uv_uitreeobject_st ** const objs = (uv_uitreeobject_st ** const) ((uv_uiwindow_st*) this)->objects;
	for (int i = 0; i < ((uv_uiwindow_st*) this)->objects_count; i++) {
		uv_uibb(objs[i])->y = content_height;
		content_height += (((uv_uiobject_st*) objs[i])->enabled) ?
				uv_uibb(objs[i])->height : CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
	}
	uv_uiwindow_set_contentbb(this, uv_uibb(this)->width, content_height);
}



#endif


