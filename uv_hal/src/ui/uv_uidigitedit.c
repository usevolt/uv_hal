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



#include "ui/uv_uidigitedit.h"


#if CONFIG_UI

static uv_uiobject_ret_e uv_uidigitedit_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb);
static void draw(void *me, const uv_bounding_box_st *pbb);
static void touch(void *me, uv_touch_st *touch);


#define TITLE_OFFSET	4

#define this ((uv_uidigitedit_st *) me)


void uv_uidigitedit_init(void *me, uv_font_st *font,
		color_t color, uint32_t value, const uv_uistyle_st *style) {
	uv_uilabel_init(this, font, ALIGN_CENTER, color, "");
	this->style = style;

	uv_uiobject_set_step_callb(this, &uv_uidigitedit_step);
	uv_uiobject_set_draw_callb(this, &draw);
	uv_uiobject_set_touch_callb(this, &touch);
	this->value = !value;
	this->changed = false;
	this->numpaddialog_title = "";
	this->title = NULL;
	this->bg_color = style->active_bg_c;
	this->limit_max = INT32_MAX;
	uv_uidigitedit_set_value(this, value);

}


void uv_uidigitedit_set_value(void *me, uint32_t value) {
	if (this->value != value) {
		sprintf(this->str, "%u", (unsigned int) value);
		uv_uilabel_set_text(this, this->str);
		uv_ui_refresh(this);
		this->changed = true;
	}
	this->value = value;
}


static void draw(void *me, const uv_bounding_box_st *pbb) {
	uint16_t x = uv_ui_get_xglobal(this),
			y = uv_ui_get_yglobal(this);

	uint16_t height = uv_ft81x_get_string_height(((uv_uilabel_st*) this)->str,
			((uv_uilabel_st*) this)->font) +
					uv_ft81x_get_font_height(((uv_uilabel_st*) this)->font);
	if (height > uv_uibb(this)->height) {
		height = uv_uibb(this)->height;
	}
	else {
		y += (uv_uibb(this)->height - height) / 2;
	}
	uv_ft81x_draw_shadowrrect(x, y, uv_uibb(this)->width, height, 0, this->bg_color,
			uv_uic_brighten(this->bg_color, -30), uv_uic_brighten(this->bg_color, 30));

	_uv_uilabel_draw(this, pbb);

	if (this->title) {
		uv_ft81x_draw_string(this->title, ((uv_uilabel_st*) this)->font, x + uv_uibb(this)->width / 2,
				y + height + TITLE_OFFSET, ALIGN_TOP_CENTER, ((uv_uilabel_st*) this)->color);
	}
}


static uv_uiobject_ret_e uv_uidigitedit_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	uv_uilabel_step(this, step_ms, pbb);

	this->changed = false;

	return ret;
}



static void touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		touch->action = TOUCH_NONE;
		uint32_t value = uv_uinumpaddialog_exec( this->numpaddialog_title,
				this->limit_max, this->value, this->style);
		uv_uidigitedit_set_value(this, value);
		uv_ui_refresh(this);
	}
}



#endif
