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
	this->main_c = style->bg_c;
	this->text_c = style->text_color;
	this->text = text;
	this->font = style->font;
	uv_delay_init(&this->delay, CONFIG_UI_BUTTON_LONGPRESS_DELAY_MS);

	uv_uiobject_set_draw_callb(this, &uv_uibutton_draw);
	uv_uiobject_set_touch_callb(this, &_uv_uibutton_touch);
	((uv_uiobject_st*) this)->step_callb = &uv_uibutton_step;
}



void uv_uibutton_draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;
	color_t bgc = (this->state) ? uv_uic_brighten(this->main_c, 20) : this->main_c;
	color_t shadowc = (this->state) ? uv_uic_brighten(this->main_c, 30) : uv_uic_brighten(this->main_c, -30);
	color_t lightc = (this->state) ? uv_uic_brighten(this->main_c, -30) : uv_uic_brighten(this->main_c, 30);
	color_t fontc = this->text_c;


	uv_ft81x_draw_shadowrrect(x, y, w, h, CONFIG_UI_RADIUS,
			 bgc, lightc, shadowc);
	uv_ft81x_draw_string(this->text, this->font, x + w / 2,
			y + h / 2, ALIGN_CENTER, fontc);
}

uv_uiobject_ret_e uv_uibutton_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (uv_ui_get_enabled(this)) {
		if (((uv_uiobject_st*) this)->refresh) {
			((uv_uiobject_st*) this)->vrtl_draw(this, pbb);
			((uv_uiobject_st*) this)->refresh = false;
			ret = UIOBJECT_RETURN_REFRESH;
		}
	}

	if ((this->state == UIBUTTON_PRESSED) &&
			uv_delay(&this->delay, step_ms)) {
		this->state = UIBUTTON_LONGPRESSED;
		uv_delay_init(&this->delay, CONFIG_UI_BUTTON_LONGPRESS_DELAY_MS);
	}

	return ret;
}


void _uv_uibutton_touch(void *me, uv_touch_st *touch) {
	if (uv_ui_get_enabled(this)) {
		if (touch->action == TOUCH_IS_DOWN) {
			if ((this->state != UIBUTTON_PRESSED) &&
					(this->state != UIBUTTON_LONGPRESSED)) {
				uv_ui_refresh(this);
			}
			if (this->state != UIBUTTON_LONGPRESSED) {
				this->state = UIBUTTON_PRESSED;
			}
		}
		else if (touch->action == TOUCH_CLICKED) {
			// prevent touch action propagating to other elements
			touch->action = TOUCH_NONE;
			this->state = UIBUTTON_CLICKED;
		}
		else {
			if (this->state != UIBUTTON_UP) {
				uv_ui_refresh(this);
			}
			this->state = UIBUTTON_UP;
		}
	}

}


void uv_uibutton_set_text(void *me, char *text) {
	if (strcmp(this->text, text) != 0) {
		uv_ui_refresh(this);
	}
	this->text = text;
}


#endif
