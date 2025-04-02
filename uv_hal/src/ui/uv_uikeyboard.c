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



/// @brief: Lists all characters shown on the keyboard
/// *space* is the only special character which is shown below these
static const char letters[] = "1234567890qwertyuiopasdfghjklzxcvbnm";
static const char shift_letters[] = "1234567890QWERTYUIOPASDFGHJKLZXCVBNM";

typedef struct {
	EXTENDS(uv_uidisplay_st);

	uv_uiobject_st *bfr[1];
	bool shift;
	const char *title;
	char *buffer;
	const uv_uistyle_st *style;
} uv_uikeyboard_st;

static uv_uikeyboard_st *this;

/// @brief: Defines the length of each character line. e.g. How many buttons
/// are shown on each line
static const uint8_t line_lengths[] = {10, 10, 9, 7};

/// @brief: defines the count of the lines on keyboard
/// @note: Last line is reserved for space
#define LINE_COUNT			5

/// @brief: Defines the height of the keyboard relative to the full display height
#define KEYBOARD_HEIGHT_PPT		750

/// @brief: Defines the button height
#define BUTTON_H			(LCD_HPPT(KEYBOARD_HEIGHT_PPT) / LINE_COUNT)
/// @brief: Defines the button width
/// @note: This is relative to the maximum line lengths!
#define BUTTON_W			(LCD_W_PX / 12)

#define BUTTONS_START		(LCD_HPPT((int32_t) 1000 - KEYBOARD_HEIGHT_PPT) - 1)



/// @brief: Defines the special characters on the screen
enum {
	SHIFT = 250,
	BACKSPACE = 252,
	ENTER = 253
};

static void update_input(char *input, const uv_uistyle_st *style);



static void draw(void *me, const uv_bounding_box_st *pbb) {
	// background
	uv_ui_clear(this->style->window_c);

	uv_ui_draw_string((char*) this->title, this->style->font,
			LCD_WPPT(500), 0, ALIGN_TOP_CENTER, this->style->text_color);

	// draw current text
	update_input(this->buffer, this->style);

	// draw buttons
	uint8_t line = 0;
	uint8_t line_counter = 0;
	int16_t x = 0;
	int16_t y = BUTTONS_START;
	color_t highlight_c = uv_uic_brighten(this->style->bg_c, 30);
	color_t shadow_c = uv_uic_brighten(this->style->bg_c, -30);
	char str[2];
	for (int16_t i = 0; i < strlen(letters); i++) {
		str[0] = this->shift ? shift_letters[i] : letters[i];
		str[1] = '\0';
		uv_ui_draw_shadowrrect(x, y, BUTTON_W, BUTTON_H, CONFIG_UI_RADIUS,
				this->style->bg_c, highlight_c, shadow_c);
		uv_ui_draw_string(str, this->style->font, x + BUTTON_W / 2, y + BUTTON_H / 2,
				ALIGN_CENTER, this->style->text_color);
		x += BUTTON_W;

		line_counter++;

		// draw backspace
		if (line == 0 && line_counter >= line_lengths[line]) {
			uv_ui_draw_shadowrrect(x, y, BUTTON_W * 2, BUTTON_H, CONFIG_UI_RADIUS,
					this->style->bg_c, highlight_c, shadow_c);
			uv_ui_draw_string("Backspace", this->style->font,
					x + BUTTON_W, y + BUTTON_H / 2,
					ALIGN_CENTER, this->style->text_color);
		}
		// draw enter
		else if (line == 1 && line_counter >= line_lengths[line]) {
			uv_ui_draw_shadowrrect(x, y, BUTTON_W * 1.5, BUTTON_H * 2 + 1, CONFIG_UI_RADIUS,
					this->style->bg_c, highlight_c, shadow_c);
			uv_ui_draw_string("Enter", this->style->font,
					x + BUTTON_W * 0.75, y + BUTTON_H,
					ALIGN_CENTER, this->style->text_color);
		}
		// draw shift
		else if (line == 3 && line_counter >= line_lengths[line]) {
		uv_ui_draw_shadowrrect(x, y, BUTTON_W * 2, BUTTON_H, CONFIG_UI_RADIUS,
				this->shift ? highlight_c : this->style->bg_c,
						highlight_c, shadow_c);
		uv_ui_draw_string("Shift", this->style->font, x + BUTTON_W, y + BUTTON_H / 2,
				ALIGN_CENTER, this->style->text_color);
		}
		else {

		}

		if (line_counter >= line_lengths[line]) {
			line_counter = 0;
			line++;
			x = BUTTON_W / 2 * line;
			y += BUTTON_H;
		}
	}
	// draw space bar
	uv_ui_draw_shadowrrect(LCD_WPPT(100), y, LCD_WPPT(800), BUTTON_H, CONFIG_UI_RADIUS,
			this->style->bg_c, highlight_c, shadow_c);
	uv_ui_draw_string("Space", this->style->font, LCD_WPPT(500), y + BUTTON_H / 2,
			ALIGN_CENTER, this->style->text_color);

	// draw touch indicator
	uv_uidisplay_draw_touch_ind(this);

	// update the ft81x display
	uv_ui_dlswap();
}



/// @brief: Parses the press action and returns the character pressed. takes also care
/// of touch release events.
///
/// @param: character pressed. If no characters have been pressed, returns 0
static char get_press(uv_touch_st *touch, const uv_uistyle_st *style) {
	int16_t x, y;
	uint16_t line = 0, line_counter = 0;
	y = BUTTONS_START;
	x = 0;
	color_t highlight_c = uv_uic_brighten(style->bg_c, 30);
	color_t shadow_c = uv_uic_brighten(style->bg_c, -30);
	if (touch->action == TOUCH_NONE) {
		return '\0';
	}
	for (int16_t i = 0; i < strlen(letters); i++) {
		line_counter++;

		if (touch->x >= x && touch->x <= x + BUTTON_W &&
				touch->y >= y && touch->y <= y + BUTTON_H) {
			if (touch->action == TOUCH_CLICKED) {
				uv_ui_refresh(this);
				return this->shift ? shift_letters[i] : letters[i];
			}
		}
		else {
			if (line == 0 && line_counter >= line_lengths[line]) {
				if (touch->x >= x &&
						touch->y >= y && touch->y <= y + BUTTON_H) {
					if (touch->action == TOUCH_CLICKED) {
						uv_ui_refresh(this);
						return BACKSPACE;
					}
				}
			}
			else if (line == 1 && line_counter >= line_lengths[line]) {
				if (touch->x >= x + BUTTON_W &&
						touch->y >= y && touch->y <= y + BUTTON_H * 2) {
					if (touch->action == TOUCH_CLICKED) {
						uv_ui_refresh(this);
						return ENTER;
					}
				}
			}
			else if (line == 3 && line_counter >= line_lengths[line]) {
				if (touch->x >= x &&
						touch->y >= y && touch->y <= y + BUTTON_H) {
					if (touch->action == TOUCH_CLICKED) {
						uv_ui_refresh(this);
						return SHIFT;
					}
				}
			}
		}
		x += BUTTON_W;

		if (line_counter >= line_lengths[line]) {
			line_counter = 0;
			line++;
			x = BUTTON_W / 2 * line;
			y += BUTTON_H;
		}
	}

	if (touch->x >= LCD_WPPT(100) && touch->x <= LCD_WPPT(900) &&
			touch->y >= y) {
		if (touch->action == TOUCH_CLICKED) {
			uv_ui_refresh(this);
			return ' ';
		}
	}
	return '\0';
}


static void update_input(char *input, const uv_uistyle_st *style) {
	// clear all previous texts
	uv_ui_draw_string(input, style->font, LCD_WPPT(500), style->font->char_height,
			ALIGN_TOP_CENTER, style->text_color);

}



bool uv_uikeyboard_show(const char *title, char *buffer,
		uint16_t buf_len, const uv_uistyle_st *style) {
	bool ret;
	uv_uikeyboard_st me;
	this = &me;
	uv_uidisplay_init(this, this->bfr, &uv_uistyles[0]);
	uv_uiobject_set_draw_callb(this, &draw);
	this->shift = true;
	this->title = title;
	this->buffer = buffer;
	this->style = style;
	bool nullterm = false;
	for (uint8_t i = 0; i < buf_len; i++) {
		if (buffer[i] == '\0') {
			nullterm = true;
			break;
		}
	}
	if (!nullterm) {
		// buffer doesn't contain a null-terminated string,
		// initialize it to zero-length string
		buffer[0] = '\0';
	}
	uint16_t input_len = strlen(buffer);

	while (true) {
		uint16_t step_ms = 20;
		uv_uidisplay_step(this, step_ms);

		char c = get_press(uv_uidisplay_get_touch(this), style);
		if (c) {
			if ((uint8_t) c == SHIFT) {
				this->shift = !this->shift;
			}
			else if ((uint8_t) c == ENTER) {
				// replace added new lines with spaces
				for (int16_t i = 0; i < strlen(buffer); i++) {
					if (buffer[i] == '\n') buffer[i] = ' ';
				}
				ret = input_len ? true : false;
				break;
			}
			else if ((uint8_t) c == BACKSPACE) {
				if (input_len) {
					input_len--;
				}
				else {
					// first character defaults to uppercase
					this->shift = true;
				}
				this->buffer[input_len] = '\0';
				update_input(this->buffer, this->style);
			}
			// normal character pressed
			else {
				if (input_len < buf_len - 1) {
					this->buffer[input_len++] = c;
					this->buffer[input_len] = '\0';
					if (input_len == 1) {
						this->shift = false;
					}
					update_input(this->buffer, this->style);
				}
			}
		}

		uv_rtos_task_delay(step_ms);
	}

	return ret;
}





#endif
