/*
 * uv_uimediabutton.c
 *
 *  Created on: Nov 1, 2016
 *      Author: usevolt
 */



#include "ui/uv_uimediabutton.h"



#if CONFIG_UI

#define this ((uv_uimediabutton_st*)me)



static inline void draw(void *me, const uv_bounding_box_st *pbb) {
	color_t fontc = ((uv_uibutton_st *) this)->text_c;
	color_t bgc = (uv_uimediabutton_is_down(this)) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 20) :
			((uv_uibutton_st*) this)->main_c;
	color_t shadowc = (uv_uimediabutton_is_down(this)) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 30) :
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -30);
	color_t lightc = (uv_uimediabutton_is_down(this)) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -30) :
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 30);
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;

	uint8_t offset = 10;
	uv_ft81x_draw_shadowrrect(x, y, w, h, CONFIG_UI_RADIUS, bgc, lightc, shadowc);
	if (this->align == UIMEDIABUTTON_ALIGN_HORIZONTAL) {
		int16_t imgw = uv_uimedia_get_bitmapwidth(this->media);
		uv_ft81x_draw_bitmap(this->media, x + offset, y + h / 2 - uv_uimedia_get_bitmapheight(this->media) / 2);
		uv_ft81x_draw_string(this->super.text, ((uv_uibutton_st*) this)->font, x + imgw + (w - imgw) / 2,
				y + h / 2, ALIGN_CENTER, fontc);
	}
	else {
		int16_t contenth = uv_uimedia_get_bitmapheight(this->media) +
				uv_ft81x_get_string_height(((uv_uibutton_st*) this)->text, ((uv_uibutton_st*)this)->font);
		int16_t space = (uv_uibb(me)->height - contenth) / ((*((uv_uibutton_st*) this)->text == '\0') ? 2 : 3);
		uv_ft81x_draw_bitmap(this->media, x + w / 2 - uv_uimedia_get_bitmapwidth(this->media) / 2,
				y + space);
		uv_ft81x_draw_string(this->super.text, ((uv_uibutton_st*) this)->font, x + w / 2,
				y + space * 2 + uv_uimedia_get_bitmapheight(this->media),
						ALIGN_TOP_CENTER, fontc);

	}
}

void uv_uimediabutton_init(void *me, char *text,
		uv_uimedia_st *media, const uv_uistyle_st *style) {
	uv_uibutton_init(me, text, style);
	this->media = media;
	this->align = UIMEDIABUTTON_ALIGN_HORIZONTAL;
	((uv_uiobject_st*) this)->step_callb = &uv_uibutton_step;
	uv_uiobject_set_draw_callb(this, &draw);
}




void uv_uimediabutton_set_media(void *me, uv_uimedia_st *media) {
	if (this->media != media) {
		this->media = media;
		uv_ui_refresh(this);
	}
}



#endif
