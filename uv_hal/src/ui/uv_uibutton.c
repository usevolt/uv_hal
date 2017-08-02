/*
 * ubutton.c
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */


#include <ui/uv_uibutton.h>
#include "ui/uv_uilabel.h"

#if CONFIG_UI


#define this ((uv_uibutton_st*)me)

void uv_uibutton_init(void *me, char *text, const uv_uistyle_st *style) {
	uv_uiobject_init(me);
	this->state = UIBUTTON_UP;
	this->style = style;
	this->text = text;
	((uv_uiobject_st*) this)->step_callb = &uv_uibutton_step;
}



static inline void draw(void *me, uint16_t step_ms, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;
	color_t bgc = (this->state) ? this->style->active_bg_c : this->style->inactive_bg_c;
	color_t shadowc = (this->state) ? this->style->highlight_c : this->style->shadow_c;
	color_t lightc = (this->state) ? this->style->shadow_c : this->style->highlight_c;
	color_t fontc = (this->state) ? this->style->active_font_c : this->style->inactive_font_c;


#if CONFIG_LCD
	color_t framec = (this->state) ? this->style->active_frame_c : this->style->inactive_frame_c;

	uv_lcd_draw_mrect(x, y, w, h, bgc, pbb);
	uv_lcd_draw_mframe(x, y, w, h, 1, framec, pbb);

	_uv_ui_draw_mtext(uv_ui_get_xglobal(this) + uv_ui_get_bb(this)->width / 2,
			uv_ui_get_yglobal(this) + uv_ui_get_bb(this)->height / 2,
			this->style->font, ALIGN_CENTER, fontc, bgc, this->text, 1.0f, pbb);

#elif CONFIG_FT81X
	uv_ft81x_draw_rrect(x, y, w - 4, h - 4, CONFIG_UI_RADIUS, shadowc);
	uv_ft81x_draw_rrect(x + 4, y + 4, w - 4, h - 4, CONFIG_UI_RADIUS, lightc);
	uv_ft81x_draw_rrect(x + 2, y + 2, w - 4, h - 4, CONFIG_UI_RADIUS, bgc);
	uv_ft81x_draw_string(this->text, this->style->font->index, x + w / 2,
			y + h / 2, ALIGN_CENTER, fontc);
#endif
}

uv_uiobject_ret_e uv_uibutton_step(void *me, uv_touch_st *touch,
		uint16_t step_ms, const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (uv_ui_get_enabled(this)) {
		if (((uv_uiobject_st*) this)->refresh) {
			draw(this, step_ms, pbb);
			((uv_uiobject_st*) this)->refresh = false;
			ret = UIOBJECT_RETURN_REFRESH;
		}


		if (touch->action == TOUCH_IS_DOWN) {
			if (this->state != UIBUTTON_PRESSED) {
				uv_ui_refresh(this);
			}
			this->state = UIBUTTON_PRESSED;
		}
		else if (touch->action == TOUCH_CLICKED) {
			// prevent touch action propagating to other elements
			touch->action = TOUCH_NONE;
			this->state = UIBUTTON_CLICKED;
		}
		else if (touch->action == TOUCH_LONG_PRESSED) {
			this->state = UIBUTTON_LONGPRESSED;
			// prevent touch action propagating to other elements
			touch->action = TOUCH_NONE;
		}
		else {
			if (this->state != UIBUTTON_UP) {
				uv_ui_refresh(this);
			}
			this->state = UIBUTTON_UP;
		}
	}
	return ret;
}

#endif
