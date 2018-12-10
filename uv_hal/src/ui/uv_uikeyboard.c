/*
 * uv_uikeyboard.c
 *
 *  Created on: Oct 27, 2016
 *      Author: usevolt
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
	uv_ft81x_clear(style->window_c);

	uv_ft81x_draw_string((char*) title, style->font,
			LCD_W(0.5), 0, ALIGN_TOP_CENTER, style->text_color);

	// draw current text
	update_input(buffer, style);

	// draw buttons
	uint8_t line = 0;
	uint8_t line_counter = 0;
	int16_t x = 0;
	int16_t y = BUTTONS_START;
	char str[2];
	for (int16_t i = 0; i < strlen(letters); i++) {
		str[0] = shift ? shift_letters[i] : letters[i];
		str[1] = '\0';
		uv_ft81x_draw_shadowrrect(x, y, BUTTON_W, BUTTON_H, CONFIG_UI_RADIUS,
				style->inactive_bg_c, style->highlight_c, style->shadow_c);
		uv_ft81x_draw_string(str, style->font, x + BUTTON_W / 2, y + BUTTON_H / 2,
				ALIGN_CENTER, style->text_color);
		x += BUTTON_W;

		line_counter++;

		// draw backspace
		if (line == 0 && line_counter >= line_lengths[line]) {
			uv_ft81x_draw_shadowrrect(x, y, BUTTON_W * 2, BUTTON_H, CONFIG_UI_RADIUS,
					style->inactive_bg_c, style->highlight_c, style->shadow_c);
			uv_ft81x_draw_string("Backspace", style->font, x + BUTTON_W, y + BUTTON_H / 2,
					ALIGN_CENTER, style->text_color);
		}
		// draw enter
		else if (line == 1 && line_counter >= line_lengths[line]) {
			uv_ft81x_draw_shadowrrect(x, y, BUTTON_W * 1.5, BUTTON_H * 2 + 1, CONFIG_UI_RADIUS,
					style->inactive_bg_c, style->highlight_c, style->shadow_c);
			uv_ft81x_draw_string("Enter", style->font, x + BUTTON_W * 0.75, y + BUTTON_H,
					ALIGN_CENTER, style->text_color);
		}
		// draw shift
		else if (line == 3 && line_counter >= line_lengths[line]) {
		uv_ft81x_draw_shadowrrect(x, y, BUTTON_W * 2, BUTTON_H, CONFIG_UI_RADIUS,
				shift ? style->active_bg_c : style->inactive_bg_c,
						style->highlight_c, style->shadow_c);
		uv_ft81x_draw_string("Shift", style->font, x + BUTTON_W, y + BUTTON_H / 2,
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
	uv_ft81x_draw_shadowrrect(LCD_W(0.1), y, LCD_W(0.8), BUTTON_H, CONFIG_UI_RADIUS,
			style->inactive_bg_c, style->highlight_c, style->shadow_c);
	uv_ft81x_draw_string("Space", style->font, LCD_W(0.5), y + BUTTON_H / 2,
			ALIGN_CENTER, style->text_color);

	// update the ft81x display
	uv_ft81x_dlswap();
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
				uv_ft81x_draw_shadowrrect(x, y, BUTTON_W, BUTTON_H, CONFIG_UI_RADIUS,
						style->active_bg_c, style->highlight_c, style->shadow_c);
				uv_ft81x_draw_string(str, style->font, x + BUTTON_W / 2, y + BUTTON_H / 2,
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
						uv_ft81x_draw_shadowrrect(x + BUTTON_W, y, BUTTON_W * 2, BUTTON_H,
								CONFIG_UI_RADIUS, style->active_bg_c, style->highlight_c, style->shadow_c);
						uv_ft81x_draw_string("Backspace", style->font,
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
						uv_ft81x_draw_shadowrrect(x + BUTTON_W, y, BUTTON_W * 1.5,
								BUTTON_H * 2, CONFIG_UI_RADIUS, style->active_bg_c,
								style->highlight_c, style->shadow_c);
						uv_ft81x_draw_string("Enter", style->font,
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
						uv_ft81x_draw_shadowrrect(x + BUTTON_W, y, BUTTON_W * 2, BUTTON_H, CONFIG_UI_RADIUS,
								style->active_bg_c, style->highlight_c, style->shadow_c);
						uv_ft81x_draw_string("Shift", style->font,
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
			uv_ft81x_draw_shadowrrect(LCD_W(0.1), y, LCD_W(0.8), BUTTON_H, CONFIG_UI_RADIUS,
					style->active_bg_c, style->highlight_c, style->shadow_c);
			uv_ft81x_draw_string("Space", style->font, LCD_W(0.5), y + BUTTON_H / 2,
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
	uv_ft81x_draw_string(input, style->font, LCD_WPPT(500), style->font->char_height,
			ALIGN_TOP_CENTER, style->text_color);

}



bool uv_uikeyboard_show(const char *title, char *buffer,
		uint16_t buf_len, const uv_uistyle_st *style) {

	uv_touch_st t;
	uint16_t input_len = 0;
	shift = true;
	bool pressed = uv_ft81x_get_touch(&t.x, &t.y);
	refresh = true;
	buffer[0] = '\0';

	while (true) {

		bool state = uv_ft81x_get_touch(&t.x, &t.y);
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
			if (c == SHIFT) {
				shift = !shift;
			}
			else if (c == ENTER) {
				// replace added new lines with spaces
				for (int16_t i = 0; i < strlen(buffer); i++) {
					if (buffer[i] == '\n') buffer[i] = ' ';
				}
				return input_len ? true : false;
			}
			else if (c == BACKSPACE) {
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

}





#endif
