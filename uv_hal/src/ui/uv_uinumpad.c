/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ui/uv_uikeyboard.h"
#include <string.h>
#include "uv_rtos.h"

#if CONFIG_UI

#define this ((uv_uinumpad_st*) me)

static void draw(void *me, const uv_bounding_box_st *pbb);
static void touch(void *me, uv_touch_st *touch);
static uv_uiobject_ret_e numpad_step(void *me, uint16_t step_ms, const uv_bounding_box_st * pbb);
static void add_number(void *me, char value);


static void add_number(void *me, char value) {
	if (strlen(this->value_str) < NUMPAD_VALUE_STR_LEN - 1) {
		char str[2];
		str[0] = value;
		str[1] = '\0';
		strcat(this->value_str, str);
		this->value = strtol(this->value_str, NULL, 0);
		uv_ui_refresh(this);
	}
}


void uv_uinumpad_init(void *me, const char *title, const uv_uistyle_st *style) {
	uv_uiobject_init(this);
	uv_uiobject_set_draw_callb(this, &draw);
	uv_uiobject_set_touch_callb(this, &touch);
	uv_uiobject_set_step_callb(this, &numpad_step);

	this->title = title;
	this->style = style;
	this->value = -1;
	this->pressed_index = -1;
	this->released_index = -1;
	this->cancelled = false;
	this->submitted = false;
	strcpy(this->value_str, "");
}


const char *labels[] = {
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		"0"
};

static void draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t globx = uv_ui_get_xglobal(this);
	int16_t globy = uv_ui_get_yglobal(this);
	int16_t butw = uv_uibb(this)->width / 3;
	int16_t buth = uv_uibb(this)->height / 5;

	uv_ft81x_draw_string((char*) this->title, this->style->font,
			globx + uv_uibb(this)->width / 2, globy, ALIGN_TOP_CENTER, this->style->text_color);

	if (strlen(this->value_str)) {
		uv_ft81x_draw_string(this->value_str, this->style->font,
				globx + uv_uibb(this)->width / 2, globy + buth / 2,
				ALIGN_TOP_CENTER, this->style->text_color);
	}

	uint8_t index = 0;
	for (int8_t y = 1; y < 5; y++) {
		for (int8_t x = 0; x < 3; x++) {
			uv_ft81x_draw_shadowrrect(globx + x * butw, globy + y * buth, butw, buth, CONFIG_UI_RADIUS,
					(this->pressed_index == ((y - 1) * 3 + x)) ? this->style->active_bg_c : this->style->inactive_bg_c,
					this->style->highlight_c, this->style->shadow_c);
			if (y == 4 && x == 0) {
				uv_ft81x_draw_string("Back", this->style->font,
						globx + x * butw + butw / 2, globy + y * buth + buth / 2,
						ALIGN_CENTER, this->style->text_color);
			}
			else if (y == 4 && x == 2) {
				uv_ft81x_draw_string("OK", this->style->font,
						globx + x * butw + butw / 2, globy + y * buth + buth / 2,
						ALIGN_CENTER, this->style->text_color);
			}
			else {
				uv_ft81x_draw_string((char*) labels[index], this->style->font,
						globx + x * butw + butw / 2, globy + y * buth + buth / 2,
						ALIGN_CENTER, this->style->text_color);
				index++;
			}

		}
	}
}


static void touch(void *me, uv_touch_st *touch) {
	int16_t butw = uv_uibb(this)->width / 3;
	int16_t buth = uv_uibb(this)->height / 5;

	if (touch->action == TOUCH_PRESSED) {
		uint8_t index = 0;
		bool found = false;
		for (uint8_t y = 1; y < 5; y++) {
			for (uint8_t x = 0; x < 3; x++) {
				if ((touch->y >= y * buth) &&
						(touch->y < (y + 1) * buth)) {
					if ((touch->x >= x * butw) &&
							(touch->x < (x + 1) * butw)) {
						this->pressed_index = index;
						uv_ui_refresh(this);
						found = true;
						break;
					}
				}
				index++;
			}
			if (found) {
				break;
			}
		}
	}
	else if ((touch->action == TOUCH_RELEASED) ||
			(touch->action == TOUCH_CLICKED)) {
		this->released_index = this->pressed_index;
		this->pressed_index = -1;
		uv_ui_refresh(this);
	}
	else {

	}
}

static uv_uiobject_ret_e numpad_step(void *me, uint16_t step_ms, const uv_bounding_box_st * pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (((uv_uiobject_st*) this)->refresh) {
		draw(this, pbb);
		((uv_uiobject_st*) this)->refresh = false;
	}

	this->cancelled = false;
	this->submitted = false;

	// back pressed
	if (this->released_index == 9) {
		uint8_t len = strlen(this->value_str);
		if (len) {
			this->value_str[len - 1] = '\0';
			if (strlen(this->value_str)) {
				this->value = strtol(this->value_str, NULL, 0);
			}
			else {
				this->value = -1;
			}
			uv_ui_refresh(this);
		}
		else {
			this->cancelled = true;
		}
	}
	// 0 pressed
	else if (this->released_index == 10) {
		if (this->value != 0) {
			add_number(this, '0');
		}
	}
	// ok pressed
	else if (this->released_index == 11) {
		this->submitted = true;
	}
	// other numbers pressed
	else if (this->released_index != -1) {
		// remove MSB zero
		if (this->value == 0) {
			strcpy(this->value_str, "");
		}
		add_number(this, '0' + this->released_index + 1);
	}
	else {

	}

	this->released_index = -1;

	return ret;
}

static uv_uinumpad_st *numpad_ptr;

static uv_uiobject_ret_e exec_callb(uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (uv_uinumpad_get_cancelled(numpad_ptr) ||
			uv_uinumpad_get_submitted(numpad_ptr)) {
		// close the uidialog
		ret = UIOBJECT_RETURN_KILLED;
	}

	return ret;
}


int32_t uv_uinumpaddialog_exec(const char *title, int32_t def_value, const uv_uistyle_st *style) {
	uv_uidialog_st d;
	uv_uiobject_st *bfr;
	uv_uinumpad_st numpad;
	numpad_ptr = &numpad;
	uv_uidialog_init(&d, &bfr, style);
	uv_uidialog_set_stepcallback(&d, &exec_callb);

	uv_uinumpad_init(&numpad, title, &uv_uistyles[0]);
	uv_uidialog_add(&d, &numpad, 0, 0,
			uv_uibb(&d)->width, uv_uibb(&d)->height);

	uv_uidialog_exec(&d);

	int32_t value = uv_uinumpad_get_value(&numpad);
	if (value == -1) {
		value = def_value;
	}
	return value;
}


#endif
