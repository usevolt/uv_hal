/*
 * uv_uitogglebutton.c
 *
 *  Created on: Nov 1, 2016
 *      Author: usevolt
 */



#include "ui/uv_uitogglebutton.h"



#if CONFIG_UI

#define this ((uv_uitogglebutton_st*)me)


static void touch(void *me, uv_touch_st *touch);


static inline void draw(void *me, const uv_bounding_box_st *pbb) {
	color_t bgc = (this->state) ? this->super.style->active_bg_c : this->super.style->inactive_bg_c;
	color_t fontc = (this->super.state) ? this->super.style->active_font_c : this->super.style->inactive_font_c;
	color_t lightc = (this->state) ? this->super.style->shadow_c : this->super.style->highlight_c;
	color_t shadowc = (this->state) ? this->super.style->highlight_c : this->super.style->shadow_c;
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;

#if CONFIG_LCD
	color_t framec = (this->state) ? this->super.style->active_frame_c : this->super.style->inactive_frame_c;

	uv_lcd_draw_mrect(x, y, w, h, bgc, pbb);

	uv_lcd_draw_mframe(x, y, w, h, 1, framec, pbb);

	_uv_ui_draw_mtext(x + w / 2, y + h / 2, this->super.style->font,
			ALIGN_CENTER, fontc, bgc, this->super.text, 1.0f, pbb);

#elif CONFIG_FT81X
	uv_ft81x_draw_rrect(x, y, w - 4, h - 4, CONFIG_UI_RADIUS, shadowc);
	uv_ft81x_draw_rrect(x + 4, y + 4, w - 4, h - 4, CONFIG_UI_RADIUS, lightc);
	uv_ft81x_draw_rrect(x + 2, y + 2, w - 4, h - 4, CONFIG_UI_RADIUS, bgc);
	uv_ft81x_draw_string(this->super.text, this->super.style->font->index, x + w / 2,
			y + h / 2, ALIGN_CENTER, fontc);
#endif
}

void uv_uitogglebutton_init(void *me, bool state, char *text, const uv_uistyle_st *style) {
	uv_uibutton_init(me, text, style);
	this->state = state;
	this->clicked = false;
	((uv_uiobject_st*) this)->step_callb = uv_uitogglebutton_step;
	uv_uiobject_set_draw_callb(this, &draw);
	uv_uiobject_set_touch_callb(this, &touch);
}


uv_uiobject_ret_e uv_uitogglebutton_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	this->clicked = false;

	// update if necessary
	if (((uv_uiobject_st*) this)->refresh) {
		((uv_uiobject_st*) this)->vrtl_draw(this, pbb);
		((uv_uiobject_st*) this)->refresh = false;
		ret = UIOBJECT_RETURN_REFRESH;
	}


	return ret;
}


static void touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		this->clicked = true;
		this->state = !this->state;
		uv_ui_refresh(this);
		touch->action = TOUCH_NONE;
	}

}



#endif
