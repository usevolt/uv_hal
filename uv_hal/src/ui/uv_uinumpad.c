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

#include "ui/uv_uikeyboard.h"
#include <string.h>
#include "uv_rtos.h"

#if CONFIG_UI

#define this ((uv_uinumpad_st*) me)

static void draw(void *me, const uv_bounding_box_st *pbb);
static void touch(void *me, uv_touch_st *touch);
static uv_uiobject_ret_e numpad_step(void *me, uint16_t step_ms);
static void add_number(void *me, char value);


static void add_number(void *me, char value) {
	if (strlen(this->value_str) < NUMPAD_VALUE_STR_LEN - 1) {
		char str[2];
		str[0] = value;
		str[1] = '\0';
		strcat(this->value_str, str);
		int32_t val = strtol(this->value_str, NULL,
				(this->flags & UINUMPAD_FLAGS_HEX) ? 16 : 0);
		// dont add new character if the maximum limit would be exceeded
		if (val > this->limit_max) {
			this->value_str[strlen(this->value_str) - 1] = '\0';
			this->value = this->limit_max;
		}
		else if (val < this->limit_min) {
			this->value_str[strlen(this->value_str) - 1] = '\0';
			this->value = this->limit_min;
		}
		else {
			this->value = strtol(this->value_str, NULL,
					(this->flags & UINUMPAD_FLAGS_HEX) ? 16 : 0);
		}
		uv_ui_refresh(this);
	}
}


void uv_uinumpad_init(void *me, const char *title, const uv_uistyle_st *style) {
	uv_uiobject_init(this);
	uv_uiobject_set_draw_callb(this, &draw);
	uv_uiobject_set_touch_callb(this, &touch);
	uv_uiobject_set_step_callb(this, &numpad_step);

	this->flags = UINUMPAD_FLAGS_NONE;
	this->title = title;
	this->style = style;
	this->value = 0;
	this->sign = 1;
	this->pressed_index = -1;
	this->released_index = -1;
	this->cancelled = false;
	this->submitted = false;
	this->limit_max = INT32_MAX;
	this->limit_min = 0;
	strcpy(this->value_str, "");
}

#define SIGN_INDEX			99

const char *labels_dec[] = {
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		"CANCEL",
		"0",
		"OK"
};
const char *labels_hex[] = {
		"1",
		"2",
		"3",
		"A",
		"B",
		"4",
		"5",
		"6",
		"C",
		"D",
		"7",
		"8",
		"9",
		"E",
		"F",
		"CANCEL",
		"",
		"0",
		"",
		"OK"
};

static void draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t globx = uv_ui_get_xglobal(this);
	int16_t globy = uv_ui_get_yglobal(this);
	uint8_t row_count = 5;
	uint8_t col_count = (this->flags & UINUMPAD_FLAGS_HEX) ? 5 : 3;
	int16_t butw = uv_uibb(this)->width / col_count;
	int16_t buth = uv_uibb(this)->height / row_count;
	color_t highlight_c = uv_uic_brighten(this->style->bg_c, 30);
	color_t shadow_c = uv_uic_brighten(this->style->bg_c, -30);

	uv_ui_draw_string((char*) this->title, this->style->font,
			globx + uv_uibb(this)->width / 2, globy, ALIGN_TOP_CENTER, this->style->text_color);

	if (strlen(this->value_str)) {
		int32_t strl = uv_ui_get_string_height((char*) this->title, this->style->font);
		uv_ui_draw_string(this->value_str, this->style->font,
				globx + uv_uibb(this)->width / 2, globy + strl + (buth - strl) / 2,
				ALIGN_CENTER, this->style->text_color);
	}

	uint8_t index = 0;
	for (int8_t y = 1; y < row_count; y++) {
		for (int8_t x = 0; x < col_count; x++) {

			if (this->limit_min < 0 && y == row_count - 1 && x == 0) {
				// draw background for back & sign buttons in case if the numpad is signed
				uv_ui_draw_shadowrrect(globx + x * butw, globy + y * buth,
						butw / 2, buth, CONFIG_UI_RADIUS,
						(this->pressed_index == ((y - 1) * col_count + x)) ?
								highlight_c : this->style->bg_c,
						highlight_c, shadow_c);

				uv_ui_draw_shadowrrect(globx + x * butw + butw / 2, globy + y * buth,
						butw / 2, buth, CONFIG_UI_RADIUS,
						(this->sign == -1) ?
								highlight_c : this->style->bg_c,
						highlight_c, shadow_c);
			}
			else {
				// draw background for all other buttons except sign button
				uv_ui_draw_shadowrrect(globx + x * butw, globy + y * buth,
						butw, buth, CONFIG_UI_RADIUS,
						(this->pressed_index == ((y - 1) * col_count + x)) ?
								highlight_c : this->style->bg_c,
						highlight_c, shadow_c);
			}
			if (y == row_count - 1 && x == 0) {
				if (this->limit_min < 0) {
					uv_ui_draw_string("Back", this->style->font,
							globx + x * butw + butw / 4, globy + y * buth + buth / 2,
							ALIGN_CENTER, this->style->text_color);
					uv_ui_draw_string("+/-", this->style->font,
							globx + x * butw  + butw * 3 / 4, globy + y * buth + buth / 2,
							ALIGN_CENTER, this->style->text_color);
				}
				else {
					uv_ui_draw_string("Back", this->style->font,
							globx + x * butw + butw / 2, globy + y * buth + buth / 2,
							ALIGN_CENTER, this->style->text_color);
				}
			}
			else if (y == row_count - 1 && x == col_count - 1) {
				uv_ui_draw_string("OK", this->style->font,
						globx + x * butw + butw / 2, globy + y * buth + buth / 2,
						ALIGN_CENTER, this->style->text_color);
			}
			else {
				uv_ui_draw_string((char*) ((this->flags & UINUMPAD_FLAGS_HEX) ?
								labels_hex[index] : labels_dec[index]),
								this->style->font,
						globx + x * butw + butw / 2,
						globy + y * buth + buth / 2,
						ALIGN_CENTER, this->style->text_color);
			}
			index++;

		}
	}
}


static void touch(void *me, uv_touch_st *touch) {
	uint8_t row_count = 5;
	uint8_t col_count = (this->flags & UINUMPAD_FLAGS_HEX) ? 5 : 3;
	int16_t butw = uv_uibb(this)->width / col_count;
	int16_t buth = uv_uibb(this)->height / row_count;

	if (touch->action == TOUCH_PRESSED) {
		uint8_t index = 0;
		bool found = false;
		for (uint8_t y = 1; y < row_count; y++) {
			for (uint8_t x = 0; x < col_count; x++) {
				if ((touch->y >= y * buth) &&
						(touch->y < (y + 1) * buth)) {
					if (this->limit_min < 0 &&
							y == row_count - 1 &&
							x == 0) {
						if ((touch->x >= x * butw) &&
								(touch->x < (x + 1) * butw - butw / 2)) {
							this->pressed_index = index;
							uv_ui_refresh(this);
							found = true;
							break;
						}
						else if ((touch->x >= x * butw + butw / 2) &&
								(touch->x < (x + 1) * butw)) {
							// sign-button needs special handling
							this->pressed_index = SIGN_INDEX;
							this->sign = (this->sign > 0) ? -1 : 1;
							uv_ui_refresh(this);
							found = true;

						}
						else {

						}

					}
					else {
						if ((touch->x >= x * butw) &&
								(touch->x < (x + 1) * butw)) {
							this->pressed_index = index;
							uv_ui_refresh(this);
							found = true;
							break;
						}
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

#include "uv_terminal.h"
static uv_uiobject_ret_e numpad_step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	this->cancelled = false;
	this->submitted = false;

	const char **labels = (this->flags & UINUMPAD_FLAGS_HEX) ? labels_hex : labels_dec;
	// sign pressed
	if (this->released_index == -1) {
		// nothing pressed
	}
	else if (this->released_index == SIGN_INDEX) {
		if (this->sign > 0) {
			if (strlen(this->value_str)) {
				memmove(this->value_str, &this->value_str[1],
						MIN(strlen(this->value_str) + 1, sizeof(this->value_str)));
			}
		}
		else {
			// add the minus sign to the value
			memmove(&this->value_str[1], this->value_str,
					MIN(strlen(this->value_str) + 1, sizeof(this->value_str)));
			this->value_str[0] = '-';
		}
		this->value = strtol(this->value_str, NULL,
				(this->flags & UINUMPAD_FLAGS_HEX) ? 16 : 0);
		uv_ui_refresh(this);
	}
	// back pressed
	else if (strcmp(labels[this->released_index], "CANCEL") == 0) {
		uint8_t len = strlen(this->value_str);
		if (len) {
			if (len == 2 && this->value_str[0] == '-') {
				// make sure negative sign cannot be the only character
				len = 1;
				this->sign = 1;
				uv_ui_refresh(this);
			}
			this->value_str[len - 1] = '\0';
			if (strlen(this->value_str)) {
				this->value = strtol(this->value_str, NULL,
						(this->flags & UINUMPAD_FLAGS_HEX) ? 16 : 0);
			}
			else {
				this->value = 0;
			}
			uv_ui_refresh(this);
		}
		else {
			this->cancelled = true;
		}
	}
	// ok pressed
	else if (strcmp(labels[this->released_index], "OK") == 0) {
		this->submitted = true;
	}
	// other numbers pressed
	else if (strlen(labels[this->released_index]) != 0) {
		add_number(this, *labels[this->released_index]);
		// remove trailing zeroes
		uint8_t i = (this->sign >= 0) ? 0 : 1;
		while (this->value_str[i] == '0') {
			memmove(&this->value_str[i],
					&this->value_str[i + 1],
					strlen(this->value_str) - i);
		}
		this->value = strtol(this->value_str, NULL,
				(this->flags & UINUMPAD_FLAGS_HEX) ? 16 : 0);
	}
	else {

	}

	this->released_index = -1;

	return ret;
}

static uv_uiobject_ret_e exec_callb(void *user_ptr, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (uv_uinumpad_get_cancelled(user_ptr) ||
			uv_uinumpad_get_submitted(user_ptr)) {
		// close the uidialog
		ret = UIOBJECT_RETURN_KILLED;
	}

	return ret;
}


int32_t uv_uinumpaddialog_exec(const char *title,
		int32_t max_limit, int32_t min_limit,
		int32_t fallback_value, uv_uinumpad_flags_e flags,
		const uv_uistyle_st *style) {
	uv_uidialog_st d;
	uv_uiobject_st *bfr;
	uv_uinumpad_st numpad;
	uv_uidialog_init(&d, &bfr, style);
	uv_uidialog_set_stepcallback(&d, &exec_callb, &numpad);

	uv_uinumpad_init(&numpad, title, &uv_uistyles[0]);
	uv_uinumpad_set_flags(&numpad, flags);
	uv_uinumpad_set_maxlimit(&numpad, max_limit);
	uv_uinumpad_set_minlimit(&numpad, min_limit);
	uv_uidialog_addxy(&d, &numpad, 0, 0,
			uv_uibb(&d)->width, uv_uibb(&d)->height);

	uv_uidialog_exec(&d);

	int32_t value = uv_uinumpad_get_cancelled(&numpad) ?
			fallback_value : uv_uinumpad_get_value(&numpad);
	return value;
}


#endif
