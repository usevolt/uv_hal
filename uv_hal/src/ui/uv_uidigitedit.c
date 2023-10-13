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
static void touch(void *me, uv_touch_st *touch);
static void set_value(void *me, int32_t value, bool forced_update);


#define TITLE_OFFSET	4

#define this ((uv_uidigitedit_st *) me)


void uv_uidigitedit_init(void *me, int32_t value, const uv_uistyle_st *style) {
	uv_uilabel_init(this, style->font, ALIGN_CENTER, style->text_color, "");
	this->style = style;
	this->divider = 0;
	uv_uidigitedit_set_mode(this, UIDIGITEDIT_MODE_NORMAL);
	uv_uiobject_set_step_callb(this, &uv_uidigitedit_step);
	uv_uiobject_set_draw_callb(this, &uv_uidigitedit_draw);
	uv_uiobject_set_touch_callb(this, &touch);
	this->value = !value;
	this->title = NULL;
	this->unit = NULL;
	this->bg_color = style->bg_c;
	this->limit_max = INT32_MAX;
	this->limit_min = (value > 0) ? 0 : INT16_MIN + 1;
	uv_uidigitedit_set_value(this, value);
	// this has to be set after *uv_uidigit_set_value* to make sure
	// that the change flag is not set after init
	this->changed = false;
	memset(&this->modedata, 0, sizeof(this->modedata));
}


void uv_uidigitedit_set_mode(void *me, uv_uidigitedit_mode_e value) {
	this->mode = value;
	if (this->mode == UIDIGITEDIT_MODE_NORMAL) {
		this->modedata.normal.numpaddialog_title = "";
	}
	else if (this->mode == UIDIGITEDIT_MODE_INCDEC) {
		this->modedata.incdec.inc_step = 1;
		this->modedata.incdec.pressed_button = 0;
		uv_delay_end(&this->modedata.incdec.delay);
	}
}


void uv_uidigitedit_set_unit(void *me, char *value) {
	this->unit = value;
	set_value(this, this->value, true);
}


static void set_value(void *me, int32_t value, bool forced_update) {
	LIMITS(value, this->limit_min, this->limit_max);

	if (this->value != value ||
			forced_update) {
		if (this->divider == 0) {
			sprintf(this->str, "%i %s", (int) value, (this->unit) ? this->unit : "");
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
			strcat(format, "u %s");

			sprintf(this->str, format, (int) value / this->divider,
					(unsigned int) value % this->divider,
					(this->unit) ? this->unit : "");
		}
		uv_uilabel_set_text(this, this->str);
		uv_ui_refresh(this);
		if (this->value != value) {
			this->changed = true;
		}
	}
	this->value = value;
}

void uv_uidigitedit_set_value(void *me, int32_t value) {
	set_value(this, value, false);
}


void uv_uidigitedit_draw(void *me, const uv_bounding_box_st *pbb) {
	uint16_t x = uv_ui_get_xglobal(this),
			y = uv_ui_get_yglobal(this);

	uint16_t height = 0;
	if (this->mode == UIDIGITEDIT_MODE_NORMAL) {
		height = (uv_ui_get_string_height(((uv_uilabel_st*) this)->str,
				((uv_uilabel_st*) this)->font) +
						TITLE_OFFSET * 2);
	}
	else if (this->mode == UIDIGITEDIT_MODE_INCDEC) {
		height = CONFIG_UI_DIGITEDIT_INCDEC_BUTTON_WIDTH;
	}
	else {
		// UIDIGITEDIT_MODE_RODIGIT
		height = uv_ui_get_string_height(((uv_uilabel_st*) this)->str,
				((uv_uilabel_st*) this)->font);
	}
	int16_t title_height = (this->mode == UIDIGITEDIT_MODE_RODIGIT &&
			this->modedata.rodigit.title_font) ?
					uv_ui_get_string_height(this->title,
							this->modedata.rodigit.title_font) :
					uv_ui_get_string_height(this->title,
							((uv_uilabel_st*) this)->font);
	y += (uv_uibb(this)->h - (height + title_height + TITLE_OFFSET)) / 2;

	int16_t inc_w = (this->mode == UIDIGITEDIT_MODE_INCDEC) ?
			CONFIG_UI_DIGITEDIT_INCDEC_BUTTON_WIDTH : 0;

	if (this->mode == UIDIGITEDIT_MODE_NORMAL) {
		uv_ui_draw_shadowrrect(x, y, uv_uibb(this)->width, height, 0,
				this->bg_color, uv_uic_brighten(this->bg_color, -30),
				uv_uic_brighten(this->bg_color, 30));
	}

	if (this->mode == UIDIGITEDIT_MODE_INCDEC) {
		// draw the increment and decrement buttons
		color_t c = (this->value == this->limit_min) ?
				uv_uic_grayscale(this->bg_color) : this->bg_color;
		uv_ui_draw_shadowrrect(x, y, inc_w, height, CONFIG_UI_RADIUS, c,
				uv_uic_brighten(c, 30 * ((this->modedata.incdec.pressed_button == -1) ? -1 : 1)),
				uv_uic_brighten(c, -30 * ((this->modedata.incdec.pressed_button == -1) ? -1 : 1)));
		uv_ui_draw_string("-", ((uv_uilabel_st*) this)->font, x + inc_w / 2, y + height / 2,
				ALIGN_CENTER, ((uv_uilabel_st*) this)->color);

		c = (this->value == this->limit_max) ?
				uv_uic_grayscale(this->bg_color) : this->bg_color;
		uv_ui_draw_shadowrrect(x + uv_uibb(this)->width - inc_w, y, inc_w, height,
				CONFIG_UI_RADIUS, c,
				uv_uic_brighten(c, 30 * ((this->modedata.incdec.pressed_button == 1) ? -1 : 1)),
				uv_uic_brighten(c, -30 * ((this->modedata.incdec.pressed_button == 1) ? -1 : 1)));
		uv_ui_draw_string("+", ((uv_uilabel_st*) this)->font,
				x + uv_uibb(this)->width - inc_w / 2, y + height / 2,
				ALIGN_CENTER, ((uv_uilabel_st*) this)->color);
	}

	uv_ui_draw_string(((uv_uilabel_st*) this)->str, ((uv_uilabel_st*) this)->font,
			x + uv_uibb(this)->width / 2, y + height / 2 + CONFIG_UI_RADIUS,
			UI_ALIGN_CENTER, ((uv_uilabel_st*)this)->color);

	if (this->title) {
		uv_font_st *font = uv_uidigitedit_get_font(this);
		if (this->mode == UIDIGITEDIT_MODE_RODIGIT &&
				this->modedata.rodigit.title_font != NULL) {
			font = this->modedata.rodigit.title_font;
		}
		uv_ui_draw_string(this->title, font,
				x + uv_uibb(this)->width / 2, y + height + TITLE_OFFSET,
				ALIGN_TOP_CENTER, ((uv_uilabel_st*) this)->color);
	}
}


static uv_uiobject_ret_e uv_uidigitedit_step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	this->changed = false;

	if (this->mode == UIDIGITEDIT_MODE_INCDEC) {
		if (uv_delay(&this->modedata.incdec.delay, step_ms)) {
			uv_delay_init(&this->modedata.incdec.delay, UISLIDER_LONGPRESS_MIN_DELAY_MS);
			uv_uidigitedit_set_value(this,
					this->value + this->modedata.incdec.inc_step * this->modedata.incdec.pressed_button);
		}
	}

	return ret;
}



static void touch(void *me, uv_touch_st *touch) {
	if (this->mode == UIDIGITEDIT_MODE_NORMAL) {
		if (touch->action == TOUCH_CLICKED) {
			touch->action = TOUCH_NONE;
			uint32_t value = uv_uinumpaddialog_exec(
					this->modedata.normal.numpaddialog_title,
					this->limit_max, this->limit_min, this->value, this->style);
			uv_uidigitedit_set_value(this, value);
			uv_ui_refresh(this);
		}
	}
	else if (this->mode == UIDIGITEDIT_MODE_INCDEC) {
		if (touch->action == TOUCH_PRESSED) {
			int16_t inc_w = CONFIG_UI_DIGITEDIT_INCDEC_BUTTON_WIDTH;
			if (touch->x <= inc_w) {
				this->modedata.incdec.pressed_button = -1;
			}
			else if (touch->x > uv_uibb(this)->width - inc_w) {
				this->modedata.incdec.pressed_button = 1;
			}
			else {

			}
			uv_delay_init(&this->modedata.incdec.delay, UISLIDER_LONGPRESS_DELAY_MS);
			uv_uidigitedit_set_value(this,
					this->value + this->modedata.incdec.inc_step * this->modedata.incdec.pressed_button);
		}
		else if (touch->action != TOUCH_IS_DOWN &&
				touch->action != TOUCH_NONE) {
			uv_delay_end(&this->modedata.incdec.delay);
			this->modedata.incdec.pressed_button = 0;
			uv_ui_refresh(this);
		}
		else {

		}
	}
	else {
		// no touch for UIDIGITEDIT_MODE_RODIGIT
	}
}


void uv_uidigitedit_set_divider(void *me, uint16_t value) {
	this->divider = value;
	set_value(this, this->value, true);
}





#endif
