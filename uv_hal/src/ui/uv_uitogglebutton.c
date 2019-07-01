/*
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 *
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */



#include "ui/uv_uitogglebutton.h"



#if CONFIG_UI

#define this ((uv_uitogglebutton_st*)me)


static void touch(void *me, uv_touch_st *touch);


static inline void draw(void *me, const uv_bounding_box_st *pbb) {
	color_t fontc = ((uv_uibutton_st *) this)->text_c;
	color_t bgc = (this->state) ? uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 20) : ((uv_uibutton_st*) this)->main_c;
	color_t shadowc = (this->state) ? uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 30) :
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -30);
	color_t lightc = (this->state) ? uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -30) :
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 30);
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;

	uv_ft81x_draw_shadowrrect(x, y, w, h, CONFIG_UI_RADIUS, bgc, lightc, shadowc);
	uv_ft81x_draw_string(this->super.text, ((uv_uibutton_st*) this)->font, x + w / 2,
			y + h / 2, ALIGN_CENTER, fontc);
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
	if (_uv_uiobject_draw(this, pbb)) {
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
