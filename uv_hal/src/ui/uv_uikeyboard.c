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
static bool shift = false;
static bool refresh = true;

/// @brief: Defines the length of each character line. e.g. How many buttons
/// are shown on each line
static const uint8_t line_lengths[] = {10, 10, 9, 7};

/// @brief: defines the count of the lines on keyboard
/// @note: Last line is reserved for space
#define LINE_COUNT			5

/// @brief: Defines the height of the keyboard relative to the full display height
#define KEYBOARD_HEIGHT		0.75

/// @brief: Defines the button height
#define BUTTON_H			(LCD_H(KEYBOARD_HEIGHT) / LINE_COUNT)
/// @brief: Defines the button width
/// @note: This is relative to the maximum line lengths!
#define BUTTON_W			(LCD_W_PX / 12)

#define BUTTONS_START		(LCD_H(1 - KEYBOARD_HEIGHT) - 1)



/// @brief: Defines the special characters on the screen
enum {
	SHIFT = 250,
	BACKSPACE = 252,
	ENTER = 253
};

static void update_input(char *input, const uv_uistyle_st *style);



static void draw(const char *title, char *buffer, const uv_uistyle_st *style) {
	// background
	uv_ui_clear(style->window_c);

	uv_ui_draw_string((char*) title, style->font,
			LCD_W(0.5), 0, ALIGN_TOP_CENTER, style->text_color);

	// draw current text
	update_input(buffer, style);

	// draw buttons
	uint8_t line = 0;
	uint8_t line_counter = 0;
	int16_t x = 0;
	int16_t y = BUTTONS_START;
	color_t highlight_c = uv_uic_brighten(style->bg_c, 30);
	color_t shadow_c = uv_uic_brighten(style->bg_c, -30);
	char str[2];
	for (int16_t i = 0; i < strlen(letters); i++) {
		str[0] = shift ? shift_letters[i] : letters[i];
		str[1] = '\0';
		uv_ui_draw_shadowrrect(x, y, BUTTON_W, BUTTON_H, CONFIG_UI_RADIUS,
				style->bg_c, highlight_c, shadow_c);
		uv_ui_draw_string(str, style->font, x + BUTTON_W / 2, y + BUTTON_H / 2,
				ALIGN_CENTER, style->text_color);
		x += BUTTON_W;

		line_counter++;

		// draw backspace
		if (line == 0 && line_counter >= line_lengths[line]) {
			uv_ui_draw_shadowrrect(x, y, BUTTON_W * 2, BUTTON_H, CONFIG_UI_RADIUS,
					style->bg_c, highlight_c, shadow_c);
			uv_ui_draw_string("Backspace", style->font, x + BUTTON_W, y + BUTTON_H / 2,
					ALIGN_CENTER, style->text_color);
		}
		// draw enter
		else if (line == 1 && line_counter >= line_lengths[line]) {
			uv_ui_draw_shadowrrect(x, y, BUTTON_W * 1.5, BUTTON_H * 2 + 1, CONFIG_UI_RADIUS,
					style->bg_c, highlight_c, shadow_c);
			uv_ui_draw_string("Enter", style->font, x + BUTTON_W * 0.75, y + BUTTON_H,
					ALIGN_CENTER, style->text_color);
		}
		// draw shift
		else if (line == 3 && line_counter >= line_lengths[line]) {
		uv_ui_draw_shadowrrect(x, y, BUTTON_W * 2, BUTTON_H, CONFIG_UI_RADIUS,
				shift ? highlight_c : style->bg_c,
						highlight_c, shadow_c);
		uv_ui_draw_string("Shift", style->font, x + BUTTON_W, y + BUTTON_H / 2,
				ALIGN_CENTER, style->text_color);
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
	uv_ui_draw_shadowrrect(LCD_W(0.1), y, LCD_W(0.8), BUTTON_H, CONFIG_UI_RADIUS,
			style->bg_c, highlight_c, shadow_c);
	uv_ui_draw_string("Space", style->font, LCD_W(0.5), y + BUTTON_H / 2,
			ALIGN_CENTER, style->text_color);

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
			if (touch->action == TOUCH_PRESSED) {
				char str[2];
				str[0] = shift ? shift_letters[i] : letters[i];
				str[1] = '\0';
				uv_ui_draw_shadowrrect(x, y, BUTTON_W, BUTTON_H, CONFIG_UI_RADIUS,
						style->bg_c, highlight_c, shadow_c);
				uv_ui_draw_string(str, style->font, x + BUTTON_W / 2, y + BUTTON_H / 2,
						ALIGN_CENTER, style->text_color);
				return '\0';
			}
			else if (touch->action == TOUCH_RELEASED) {
				refresh = true;
				return shift ? shift_letters[i] : letters[i];
			}
		}
		else {
			if (line == 0 && line_counter >= line_lengths[line]) {
				if (touch->x >= x &&
						touch->y >= y && touch->y <= y + BUTTON_H) {
					if (touch->action == TOUCH_PRESSED) {
						uv_ui_draw_shadowrrect(x + BUTTON_W, y, BUTTON_W * 2, BUTTON_H,
								CONFIG_UI_RADIUS, style->bg_c, highlight_c, shadow_c);
						uv_ui_draw_string("Backspace", style->font,
								x + BUTTON_W * 2, y + BUTTON_H / 2, ALIGN_CENTER,
								style->text_color);
					}
					else if (touch->action == TOUCH_RELEASED) {
						refresh = true;
						return BACKSPACE;
					}
				}
			}
			else if (line == 1 && line_counter >= line_lengths[line]) {
				if (touch->x >= x + BUTTON_W &&
						touch->y >= y && touch->y <= y + BUTTON_H * 2) {
					if (touch->action == TOUCH_PRESSED) {
						uv_ui_draw_shadowrrect(x + BUTTON_W, y, BUTTON_W * 1.5,
								BUTTON_H * 2, CONFIG_UI_RADIUS, highlight_c,
								highlight_c, shadow_c);
						uv_ui_draw_string("Enter", style->font,
								x + BUTTON_W * 1.75, y + BUTTON_H, ALIGN_CENTER, style->text_color);
					}
					else if (touch->action == TOUCH_RELEASED) {
						refresh = true;
						return ENTER;
					}
				}
			}
			else if (line == 3 && line_counter >= line_lengths[line]) {
				if (touch->x >= x &&
						touch->y >= y && touch->y <= y + BUTTON_H) {
					if (touch->action == TOUCH_PRESSED) {
						uv_ui_draw_shadowrrect(x + BUTTON_W, y, BUTTON_W * 2, BUTTON_H, CONFIG_UI_RADIUS,
								highlight_c, highlight_c, shadow_c);
						uv_ui_draw_string("Shift", style->font,
								x + BUTTON_W * 2, y + BUTTON_H / 2, ALIGN_CENTER, style->text_color);
					}
					else if (touch->action == TOUCH_RELEASED) {
						refresh = true;
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

	if (touch->x >= LCD_W(0.1f) && touch->x <= LCD_W(0.9f) &&
			touch->y >= y) {
		if (touch->action == TOUCH_PRESSED) {
			uv_ui_draw_shadowrrect(LCD_W(0.1), y, LCD_W(0.8), BUTTON_H, CONFIG_UI_RADIUS,
					highlight_c, highlight_c, shadow_c);
			uv_ui_draw_string("Space", style->font, LCD_W(0.5), y + BUTTON_H / 2,
					ALIGN_CENTER, style->text_color);
		}
		else if (touch->action == TOUCH_RELEASED) {
			refresh = true;
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
	uv_touch_st t;
	shift = true;
	bool pressed = uv_ui_get_touch(&t.x, &t.y);
	refresh = true;
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

		bool state = uv_ui_get_touch(&t.x, &t.y);
		// either pressed or released
		if (state && !pressed) {
			t.action = TOUCH_PRESSED;
		}
		else if (!state && pressed) {
			t.action = TOUCH_RELEASED;
		}
		else {
			t.action = TOUCH_NONE;
		}
		pressed = state;

		char c = get_press(&t, style);
		if (c) {
			if ((uint8_t) c == SHIFT) {
				shift = !shift;
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
					shift = true;
				}
				buffer[input_len] = '\0';
				update_input(buffer, style);
			}
			// normal character pressed
			else {
				if (input_len < buf_len - 1) {
					buffer[input_len++] = c;
					buffer[input_len] = '\0';
					if (input_len == 1) {
						shift = false;
					}
					update_input(buffer, style);
				}
			}
		}

		if (refresh) {
			draw(title, buffer, style);
			refresh = false;
		}

		uv_rtos_task_delay(20);
	}

	return ret;
}





#endif
