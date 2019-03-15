/*
 * ubutton.c
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */


#include <ui/uv_uilistbutton.h>
#include "ui/uv_uilabel.h"

#if CONFIG_UI




#define this ((uv_uilistbutton_st*)me)


void uv_uilistbutton_draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;

	color_t bgc = (((uv_uibutton_st*) this)->state) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 20) :
				((uv_uibutton_st*) this)->main_c;
	color_t shadowc = (((uv_uibutton_st*) this)->state) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 30) :
				uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -30);
	color_t lightc = (((uv_uibutton_st*) this)->state) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -30) :
				uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 30);
	color_t fontc = ((uv_uibutton_st*) this)->text_c;


	uv_ft81x_draw_shadowrrect(x, y, w, h, CONFIG_UI_RADIUS,
			 bgc, lightc, shadowc);
	alignment_e a = (this->title) ? ALIGN_TOP_CENTER : ALIGN_CENTER;
	if (this->title) {
		uv_ft81x_draw_string(this->title, ((uv_uibutton_st*) this)->font, x + w / 2,
				y + h / 2 - uv_ft81x_get_string_height(this->title,
						((uv_uibutton_st*) this)->font), ALIGN_TOP_CENTER, fontc);
	}
	uv_ft81x_draw_string(this->content[this->current_index], ((uv_uibutton_st*) this)->font,
			x + w / 2, y + h / 2, a, fontc);

	int16_t offset = 4;
	int16_t barw = (w - offset) / (this->content_len + offset);
	for (uint8_t i = 0; i < this->content_len; i++) {
		color_t c = (this->current_index == i) ? this->activebar_c : this->bar_c;
		uv_ft81x_draw_rrect(x + offset + i * barw, y + h - offset,
				barw, CONFIG_UI_LISTBUTTON_BAR_HEIGHT, 0, c);
	}
}


void uv_uilistbutton_init(void *me, char **content,
		uint8_t content_len, uint8_t current_index, const uv_uistyle_st *style) {
	uv_uibutton_init(this, content[content_len], style);
	uv_uiobject_set_draw_callb(this, &uv_uilistbutton_draw);
	uv_uiobject_set_step_callb(this, &uv_uilistbutton_step);
	this->activebar_c = style->active_fg_c;
	this->bar_c = style->active_bg_c;
	this->content_len = content_len;
	this->content = content;
	this->current_index = current_index;

}


uv_uiobject_ret_e uv_uilistbutton_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (uv_uibutton_clicked(this)) {
		this->current_index++;
		if (this->current_index >= this->content_len) {
			this->current_index = 0;
		}
	}

	if (uv_ui_get_enabled(this)) {
		if (((uv_uiobject_st*) this)->refresh) {
			((uv_uiobject_st*) this)->vrtl_draw(this, pbb);
			((uv_uiobject_st*) this)->refresh = false;
			ret = UIOBJECT_RETURN_REFRESH;
		}
	}


	return ret;
}




#endif
