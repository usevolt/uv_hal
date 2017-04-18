/*
 * uv_uitreewindow.c
 *
 *  Created on: Apr 12, 2017
 *      Author: usevolt
 */


#include "ui/uv_uitreeview.h"


#if CONFIG_LCD


#define this ((uv_uitreeview_st*) me)


static void draw(const void *me, const uv_bounding_box_st *pbb);



static void draw(const void *me, const uv_bounding_box_st *pbb) {

	// calculate the content height
	int16_t height = CONFIG_UI_TREEVIEW_ITEM_HEIGHT * this->obj_count;
	if (this->active_object != -1) {
		height += uv_uibb(this->object_array[this->active_object].window)->height;
	}
	((uv_uiwindow_st*) this)->content_bb.height = height;


	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;

	for (uint16_t i = 0; i < this->obj_count; i++) {
		int8_t xoffset = 5;

		if (this->active_object != i) {
			_uv_ui_draw_mtext(x + xoffset, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
					this->arrow_font, ALIGN_CENTER_LEFT,
					((uv_uiwindow_st*) me)->style->active_bg_c,
					((uv_uiwindow_st*) me)->style->window_c, "\x10", 1.0f, pbb);
		}
		else {
			_uv_ui_draw_mtext(x + xoffset, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
					this->arrow_font, ALIGN_CENTER_LEFT,
					((uv_uiwindow_st*) me)->style->active_bg_c,
					((uv_uiwindow_st*) me)->style->window_c, "\x1f", 1.0f, pbb);
		}

		_uv_ui_draw_mtext(x + xoffset * 2 + this->arrow_font->char_width,
				y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
				((uv_uiwindow_st*) me)->style->font, ALIGN_CENTER_LEFT,
				((uv_uiwindow_st*) me)->style->text_color, C(0xFFFFFFFF),
				this->object_array[i].name, 1.0f, pbb);

		if (this->active_object == i) {
			y += uv_uibb(this->object_array[i].window)->height +
					CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
		}
		else {
			y += CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
		}
		uv_lcd_draw_mrect(x, y, w, 1,
				((uv_uiwindow_st*) me)->style->inactive_frame_c, pbb);
	}

	if ((this->active_object != -1) &&
			(this->object_array[this->active_object].show_callb != NULL) ) {
		// show up the active window
		printf("show\n");
		this->object_array[this->active_object].show_callb();
	}
}



void uv_uitreeview_init(void *me, const uv_uitreeobject_st *object_array, const int16_t object_count,
		const uv_font_st *arrow_font, const uv_uistyle_st * style) {
	uv_uiwindow_init(this, NULL, style);
	uv_uiwindow_set_contentbb(this, 0, object_count * CONFIG_UI_TREEVIEW_ITEM_HEIGHT);
	((uv_uiwindow_st*) me)->vrtl_draw = &draw;
	((uv_uiobject_st*) me)->step_callb = &_uv_uitreeview_step;
	this->active_object = -1;
	this->object_array = object_array;
	this->obj_count = object_count;
	this->arrow_font = arrow_font;
}



uv_uiobject_ret_e _uv_uitreeview_step(void *me, uv_touch_st *touch, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {

	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (touch->action == TOUCH_CLICKED) {
		int16_t t = touch->y + ((uv_uiwindow_st*) me)->content_bb.y;
		bool touched = true;
		if (this->active_object != -1) {
			if (t < (this->active_object * CONFIG_UI_TREEVIEW_ITEM_HEIGHT)) {

			}
			else if (t > (this->active_object * CONFIG_UI_TREEVIEW_ITEM_HEIGHT +
							uv_uibb(this->object_array[this->active_object].window)->height)) {
				t -= uv_uibb(this->object_array[this->active_object].window)->height;
			}
			else {
				// active object window was clicked, do nothing
				touched = false;
			}
		}
		if (touched) {
			t /= CONFIG_UI_TREEVIEW_ITEM_HEIGHT;

			if (this->active_object == t) {
				this->active_object = -1;
			}
			else {
				this->active_object = t;
				// set active object's width and height to it's content's width and height
				((uv_uiobject_st*) this->object_array[t].window)->bb.height =
						this->object_array[t].window->content_bb.height;
				((uv_uiobject_st*) this->object_array[t].window)->bb.width =
						this->object_array[t].window->content_bb.width;
			}
			touch->action = TOUCH_NONE;
		}
	}

	ret = uv_uiwindow_step(this, touch, step_ms, pbb);

	// call active object's step function
	if (this->active_object != -1) {
		ret |= uv_uiwindow_step(
				this->object_array[this->active_object].window,
				touch, step_ms, pbb);
	}

	return ret;

}


void uv_uitreeview_set_active(void *me, uint16_t active_index) {
	if (active_index < this->obj_count) {
		this->active_object = active_index;
		uv_ui_refresh(this);
	}
}



#endif
