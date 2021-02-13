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

static uv_uiobject_ret_e uv_uidigitedit_step(void *me, uint16_t step_ms);
static void draw(void *me, const uv_bounding_box_st *pbb);
static void touch(void *me, uv_touch_st *touch);


#define TITLE_OFFSET	4

#define this ((uv_uidigitedit_st *) me)


void uv_uidigitedit_init(void *me, uint32_t value, const uv_uistyle_st *style) {
	uv_uilabel_init(this, style->font, ALIGN_CENTER, style->text_color, "");
	this->style = style;
	this->divider = 0;
	uv_uidigitedit_set_mode(this, UIDIGITEDIT_MODE_NORMAL);
	uv_uiobject_set_step_callb(this, &uv_uidigitedit_step);
	uv_uiobject_set_draw_callb(this, &draw);
	uv_uiobject_set_touch_callb(this, &touch);
	this->value = !value;
	this->title = NULL;
	this->bg_color = style->bg_c;
	this->limit_max = INT32_MAX;
	this->limit_min = (value > 0) ? 0 : INT16_MIN + 1;
	uv_uidigitedit_set_value(this, value);
	// this has to be set after *uv_uidigit_set_value* to make sure
	// that the change flag is not set after init
	this->changed = false;
}


void uv_uidigitedit_set_mode(void *me, uv_uidigitedit_mode_e value) {
	this->mode = value;
	if (this->mode == UIDIGITEDIT_MODE_NORMAL) {
		this->normal.numpaddialog_title = "";
	}
	else if (this->mode == UIDIGITEDIT_MODE_INCDEC) {
		this->incdec.inc_step = 1;
		this->incdec.pressed_button = 0;
		uv_delay_end(&this->incdec.delay);
	}
}


void uv_uidigitedit_set_value(void *me, int32_t value) {
	LIMITS(value, this->limit_min, this->limit_max);

	if (this->value != value) {
		if (this->divider == 0) {
			sprintf(this->str, "%i", (int) value);
		}
		else {
			char format[16];
			uint8_t decimals = 0;
			uint16_t form = this->divider;
			while (form >= 10) {
				form /= 10;
				decimals++;
			}
			strcpy(format, "%i.%0");
			sprintf(format + strlen(format), "%u", decimals);
			strcat(format, "u");

			sprintf(this->str, format, (int) value / this->divider,
					(unsigned int) value % this->divider);
		}
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
		int16_t v = (uv_uibb(this)->height - height) / 2 - ((this->title) ?
				((TITLE_OFFSET + uv_ft81x_get_string_height(this->title,
						((uv_uilabel_st*) this)->font)) / 2) : 0);
		if (v > 0) {
			height += v;
		}
	}
	int16_t inc_w = (this->mode == UIDIGITEDIT_MODE_INCDEC) ?
			CONFIG_UI_DIGITEDIT_INCDEC_BUTTON_WIDTH : 0;
	uv_ft81x_draw_shadowrrect(x + inc_w, y, uv_uibb(this)->width - inc_w * 2, height, 0,
			this->bg_color, uv_uic_brighten(this->bg_color, -30),
			uv_uic_brighten(this->bg_color, 30));

	if (this->mode == UIDIGITEDIT_MODE_INCDEC) {
		// draw the increment and decrement buttons
		uv_ft81x_draw_shadowrrect(x, y, inc_w, height, CONFIG_UI_RADIUS, this->bg_color,
				uv_uic_brighten(this->bg_color, 30 *
						((this->incdec.pressed_button == -1) ? -1 : 1)),
				uv_uic_brighten(this->bg_color, -30 *
						((this->incdec.pressed_button == -1) ? -1 : 1)));
		uv_ft81x_draw_string("-", ((uv_uilabel_st*) this)->font, x + inc_w / 2, y + height / 2,
				ALIGN_CENTER, ((uv_uilabel_st*) this)->color);

		uv_ft81x_draw_shadowrrect(x + uv_uibb(this)->width - inc_w, y, inc_w, height,
				CONFIG_UI_RADIUS, this->bg_color,
				uv_uic_brighten(this->bg_color, 30 *
						((this->incdec.pressed_button == 1) ? -1 : 1)),
				uv_uic_brighten(this->bg_color, -30 *
						((this->incdec.pressed_button == 1) ? -1 : 1)));
		uv_ft81x_draw_string("+", ((uv_uilabel_st*) this)->font,
				x + uv_uibb(this)->width - inc_w / 2, y + height / 2,
				ALIGN_CENTER, ((uv_uilabel_st*) this)->color);
	}

	uv_ft81x_draw_string(((uv_uilabel_st*) this)->str, ((uv_uilabel_st*) this)->font,
			x + uv_uibb(this)->width / 2, y + height / 2,
			FT81X_ALIGN_CENTER, ((uv_uilabel_st*)this)->color);

	if (this->title) {
		uv_ft81x_draw_string(this->title, ((uv_uilabel_st*) this)->font,
				x + uv_uibb(this)->width / 2, y + height + TITLE_OFFSET,
				ALIGN_TOP_CENTER, ((uv_uilabel_st*) this)->color);
	}
}


static uv_uiobject_ret_e uv_uidigitedit_step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	this->changed = false;

	if (this->mode == UIDIGITEDIT_MODE_INCDEC) {
		if (uv_delay(&this->incdec.delay, step_ms)) {
			uv_delay_init(&this->incdec.delay, UISLIDER_LONGPRESS_MIN_DELAY_MS);
			uv_uidigitedit_set_value(this,
					this->value + this->incdec.inc_step * this->incdec.pressed_button);
		}
	}

	return ret;
}



static void touch(void *me, uv_touch_st *touch) {
	if (this->mode == UIDIGITEDIT_MODE_NORMAL) {
		if (touch->action == TOUCH_CLICKED) {
			touch->action = TOUCH_NONE;
			uint32_t value = uv_uinumpaddialog_exec( this->normal.numpaddialog_title,
					this->limit_max, this->limit_min, this->value, this->style);
			uv_uidigitedit_set_value(this, value);
			uv_ui_refresh(this);
		}
	}
	else if (this->mode == UIDIGITEDIT_MODE_INCDEC) {
		if (touch->action == TOUCH_PRESSED) {
			int16_t inc_w = CONFIG_UI_DIGITEDIT_INCDEC_BUTTON_WIDTH;
			if (touch->x <= inc_w) {
				this->incdec.pressed_button = -1;
			}
			else if (touch->x > uv_uibb(this)->width - inc_w) {
				this->incdec.pressed_button = 1;
			}
			else {

			}
			uv_delay_init(&this->incdec.delay, UISLIDER_LONGPRESS_DELAY_MS);
			uv_uidigitedit_set_value(this,
					this->value + this->incdec.inc_step * this->incdec.pressed_button);
		}
		else if (touch->action != TOUCH_IS_DOWN &&
				touch->action != TOUCH_NONE) {
			uv_delay_end(&this->incdec.delay);
			this->incdec.pressed_button = 0;
			uv_ui_refresh(this);
		}
		else {

		}
	}
	else {

	}
}



#endif
