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


#include <ui/uv_uiduallabelbutton.h>
#include "ui/uv_uilabel.h"

#if CONFIG_UI

static void uv_uiduallabelbutton_draw(void *me, const uv_bounding_box_st *pbb);

#define this ((uv_uiduallabelbutton_st*)me)

void uv_uiduallabelbutton_init(void *me, char *text,
		char *dualtext, const uv_uistyle_st *style) {
	uv_uibutton_init(me, text, style);
	uv_uiobject_set_draw_callb(this, &uv_uiduallabelbutton_draw);
	this->dualfont = style->font;
	this->dualtext = dualtext;
	this->dualtext_c = style->text_color;
}



static void uv_uiduallabelbutton_draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;
	color_t bgc = (((uv_uibutton_st*) this)->state) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 20) :
			((uv_uibutton_st*) this)->main_c;
	color_t shadowc = ((uv_uibutton_st*) me)->state ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 30) :
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -30);
	color_t lightc = (((uv_uibutton_st*) this)->state) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -30) :
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 30);
	color_t fontc = ((uv_uibutton_st*) this)->text_c;


	uint8_t text_offset = 0;
	uv_ft81x_draw_shadowrrect(x, y, w, h, CONFIG_UI_RADIUS,
			 bgc, lightc, shadowc);
	uv_ft81x_draw_string(((uv_uibutton_st*) this)->text,
			((uv_uibutton_st*) this)->font, x + w / 2,
			y + h / 2 - text_offset - uv_ft81x_get_string_height(((uv_uibutton_st*) this)->text,
													((uv_uibutton_st*) this)->font),
			ALIGN_TOP_CENTER, fontc);
	uv_ft81x_draw_string(this->dualtext, this->dualfont,
			x + w / 2,
			y + h / 2 + text_offset, ALIGN_TOP_CENTER, this->dualtext_c);
}


void uv_uiduallabelbutton_set_dual_text(void *me, char *dualtext) {
	if (strcmp(this->dualtext, dualtext) != 0) {
		uv_ui_refresh(this);
	}
	this->dualtext = dualtext;
}




#endif
