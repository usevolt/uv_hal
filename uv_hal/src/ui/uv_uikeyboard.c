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
#if CONFIG_LCD
	// fill whole screen
	uv_lcd_draw_rect(0, 0, LCD_W_PX, LCD_H_PX, style->window_c);

	// draw title text
	_uv_ui_draw_text(LCD_W(0.5), 0, style->font, ALIGN_TOP_CENTER,
			style->text_color, C(0xFFFFFFFF), (char*) title, 1.0f);

#elif CONFIG_FT81X
	uv_ft81x_draw_string((char*) title, style->font->index,
			LCD_W(0.5), 0, ALIGN_TOP_CENTER, style->text_color);
#endif

	// draw current text
	update_input(buffer, style);

	// draw buttons
	// half of the display is used to display the keyboard
	uint8_t line = 0;
	uint8_t line_counter = 0;
	int16_t x = 0;
	int16_t y = BUTTONS_START;
	char str[2];
	for (int16_t i = 0; i < strlen(letters); i++) {
		str[0] = shift ? shift_letters[i] : letters[i];
		str[1] = '\0';
#if CONFIG_LCD
		uv_lcd_draw_rect(x, y, BUTTON_W, BUTTON_H, style->inactive_bg_c);
		uv_lcd_draw_frame(x, y, BUTTON_W + 1, BUTTON_H + 1, 1, style->inactive_frame_c);
		_uv_ui_draw_text(x + BUTTON_W / 2, y + BUTTON_H / 2, style->font, ALIGN_CENTER,
				style->text_color, style->inactive_bg_c, str, 1.0f);
#elif CONFIG_FT81X
		uv_ft81x_draw_shadowrrect(x, y, BUTTON_W, BUTTON_H, CONFIG_UI_RADIUS,
				style->inactive_bg_c, style->highlight_c, style->shadow_c);
		uv_ft81x_draw_string(str, style->font->index, x + BUTTON_W / 2, y + BUTTON_H / 2,
				ALIGN_CENTER, style->text_color);
#endif
		x += BUTTON_W;

		line_counter++;

		// draw backspace
		if (line == 0 && line_counter >= line_lengths[line]) {
#if CONFIG_LCD
			uv_lcd_draw_rect(x, y, BUTTON_W * 2, BUTTON_H, style->inactive_bg_c);
			uv_lcd_draw_frame(x, y, BUTTON_W * 2, BUTTON_H + 1, 1, style->inactive_frame_c);
			_uv_ui_draw_text(x + BUTTON_W, y + BUTTON_H / 2, style->font, ALIGN_CENTER,
					style->text_color, style->inactive_bg_c, "Backspace", 1.0f);
#elif CONFIG_FT81X
			uv_ft81x_draw_shadowrrect(x, y, BUTTON_W * 2, BUTTON_H, CONFIG_UI_RADIUS,
					style->inactive_bg_c, style->highlight_c, style->shadow_c);
			uv_ft81x_draw_string("Backspace", style->font->index, x + BUTTON_W, y + BUTTON_H / 2,
					ALIGN_CENTER, style->text_color);
#endif
		}
		else if (line == 1 && line_counter >= line_lengths[line]) {
			// draw enter
#if CONFIG_LCD
			uv_lcd_draw_rect(x, y, BUTTON_W * 1.5, BUTTON_H * 2 + 1, style->inactive_bg_c);
			uv_lcd_draw_frame(x, y, BUTTON_W * 1.5, BUTTON_H * 2 + 1, 1, style->inactive_frame_c);
			uv_lcd_draw_rect(x - BUTTON_W / 2 + 2, y + BUTTON_H + 1, BUTTON_W / 2, BUTTON_H - 1,
					style->inactive_bg_c);
			_uv_ui_draw_text(x + BUTTON_W * 0.75, y + BUTTON_H, style->font, ALIGN_CENTER,
					style->text_color, style->inactive_bg_c, "Enter", 1.0f);
#elif CONFIG_FT81X
			uv_ft81x_draw_shadowrrect(x, y, BUTTON_W * 1.5, BUTTON_H * 2 + 1, CONFIG_UI_RADIUS,
					style->inactive_bg_c, style->highlight_c, style->shadow_c);
			uv_ft81x_draw_string("Enter", style->font->index, x + BUTTON_W * 0.75, y + BUTTON_H,
					ALIGN_CENTER, style->text_color);
#endif
		}
		// draw shift
		else if (line == 3 && line_counter >= line_lengths[line]) {
#if CONFIG_LCD
			uv_lcd_draw_rect(x, y, BUTTON_W * 2, BUTTON_H,
					shift ? style->active_bg_c : style->inactive_bg_c);
			uv_lcd_draw_frame(x, y, BUTTON_W * 2, BUTTON_H + 1, 1,
					shift ? style->active_frame_c : style->inactive_frame_c);
			_uv_ui_draw_text(x + BUTTON_W, y + BUTTON_H / 2, style->font, ALIGN_CENTER,
					style->text_color, shift ? style->active_bg_c : style->inactive_bg_c,
							"Shift", 1.0f);
#elif CONFIG_FT81X
		uv_ft81x_draw_shadowrrect(x, y, BUTTON_W * 2, BUTTON_H, CONFIG_UI_RADIUS,
				shift ? style->active_bg_c : style->inactive_bg_c,
						style->highlight_c, style->shadow_c);
		uv_ft81x_draw_string("Shift", style->font->index, x + BUTTON_W, y + BUTTON_H / 2,
				ALIGN_CENTER, style->text_color);
#endif
		}

		if (line_counter >= line_lengths[line]) {
			line_counter = 0;
			line++;
			x = BUTTON_W / 2 * line;
			y += BUTTON_H;
		}
	}
	// draw space bar
#if CONFIG_LCD
	uv_lcd_draw_rect(LCD_W(0.1), y, LCD_W(0.8), BUTTON_H, style->inactive_bg_c);
	uv_lcd_draw_frame(LCD_W(0.1), y, LCD_W(0.8), BUTTON_H, 1, style->inactive_frame_c);
	_uv_ui_draw_text(LCD_W(0.5), y + BUTTON_H / 2, style->font, ALIGN_CENTER,
			style->text_color, style->inactive_bg_c, "Space", 1.0f);
#elif CONFIG_FT81X
	uv_ft81x_draw_shadowrrect(LCD_W(0.1), y, LCD_W(0.8), BUTTON_H, CONFIG_UI_RADIUS,
			style->inactive_bg_c, style->highlight_c, style->shadow_c);
	uv_ft81x_draw_string("Space", style->font->index, LCD_W(0.5), y + BUTTON_H / 2,
			ALIGN_CENTER, style->text_color);
#endif

#if CONFIG_LCD
	uv_lcd_double_buffer_swap();
#endif


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
#if CONFIG_LCD
				uv_lcd_draw_rect(x, y, BUTTON_W, BUTTON_H, style->active_bg_c);
				uv_lcd_draw_frame(x, y, BUTTON_W + 1, BUTTON_H + 1, 1, style->active_frame_c);
				_uv_ui_draw_text(x + BUTTON_W / 2, y + BUTTON_H / 2, style->font, ALIGN_CENTER,
						style->text_color, C(0xFFFFFFFF), str, 1.0f);
				uv_lcd_double_buffer_swap();
#elif CONFIG_FT81X
				uv_ft81x_draw_shadowrrect(x, y, BUTTON_W, BUTTON_H, CONFIG_UI_RADIUS,
						style->active_bg_c, style->highlight_c, style->shadow_c);
				uv_ft81x_draw_string(str, style->font->index, x + BUTTON_W / 2, y + BUTTON_H / 2,
						ALIGN_CENTER, style->text_color);
#endif
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
#if CONFIG_LCD
						uv_lcd_draw_rect(x + BUTTON_W, y, BUTTON_W * 2, BUTTON_H, style->active_bg_c);
						uv_lcd_draw_frame(x + BUTTON_W, y, BUTTON_W * 2 + 1, BUTTON_H + 1, 1, style->active_frame_c);
						_uv_ui_draw_text(x + BUTTON_W * 2, y + BUTTON_H / 2, style->font, ALIGN_CENTER,
								style->text_color, C(0xFFFFFFFF), "Backspace", 1.0f);
						uv_lcd_double_buffer_swap();
#elif CONFIG_FT81X
						uv_ft81x_draw_shadowrrect(x + BUTTON_W, y, BUTTON_W * 2, BUTTON_H,
								CONFIG_UI_RADIUS, style->active_bg_c, style->highlight_c, style->shadow_c);
						uv_ft81x_draw_string("Backspace", style->font->index,
								x + BUTTON_W * 2, y + BUTTON_H / 2, ALIGN_CENTER,
								style->text_color);
#endif
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
#if CONFIG_LCD
						uv_lcd_draw_rect(x + BUTTON_W, y, BUTTON_W * 1.5, BUTTON_H * 2, style->active_bg_c);
						uv_lcd_draw_frame(x + BUTTON_W, y, BUTTON_W * 1.5 + 1, BUTTON_H * 2 + 1, 1, style->active_frame_c);
						uv_lcd_draw_rect(x + BUTTON_W - BUTTON_W / 2 + 2, y + BUTTON_H + 1, BUTTON_W / 2, BUTTON_H - 1,
								style->active_bg_c);
						_uv_ui_draw_text(x + BUTTON_W * 1.75, y + BUTTON_H, style->font, ALIGN_CENTER,
								style->text_color, C(0xFFFFFFFF), "Enter", 1.0f);
						uv_lcd_double_buffer_swap();
#elif CONFIG_FT81X
						uv_ft81x_draw_shadowrrect(x + BUTTON_W, y, BUTTON_W * 1.5,
								BUTTON_H * 2, CONFIG_UI_RADIUS, style->active_bg_c,
								style->highlight_c, style->shadow_c);
						uv_ft81x_draw_string("Enter", style->font->index,
								x + BUTTON_W * 1.75, y + BUTTON_H, ALIGN_CENTER, style->text_color);
#endif
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
#if CONFIG_LCD
						uv_lcd_draw_rect(x + BUTTON_W, y, BUTTON_W * 2, BUTTON_H, style->active_bg_c);
						uv_lcd_draw_frame(x + BUTTON_W, y, BUTTON_W * 2 + 1, BUTTON_H + 1, 1, style->active_frame_c);
						_uv_ui_draw_text(x + BUTTON_W * 2, y + BUTTON_H / 2, style->font, ALIGN_CENTER,
								style->text_color, C(0xFFFFFFFF), "Shift", 1.0f);
						uv_lcd_double_buffer_swap();
#elif CONFIG_FT81X
						uv_ft81x_draw_shadowrrect(x + BUTTON_W, y, BUTTON_W * 2, BUTTON_H, CONFIG_UI_RADIUS,
								style->active_bg_c, style->highlight_c, style->shadow_c);
						uv_ft81x_draw_string("Shift", style->font->index,
								x + BUTTON_W * 2, y + BUTTON_H / 2, ALIGN_CENTER, style->text_color);
#endif
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
#if CONFIG_LCD
			uv_lcd_draw_rect(LCD_W(0.1), y, LCD_W(0.8), BUTTON_H, style->active_bg_c);
			uv_lcd_draw_frame(LCD_W(0.1), y, LCD_W(0.8), BUTTON_H, 1, style->active_frame_c);
			_uv_ui_draw_text(LCD_W(0.5), y + BUTTON_H / 2, style->font, ALIGN_CENTER,
					style->text_color, C(0xFFFFFFFF), "Space", 1.0f);
			uv_lcd_double_buffer_swap();
#elif CONFIG_FT81X
			uv_ft81x_draw_shadowrrect(LCD_W(0.1), y, LCD_W(0.8), BUTTON_H, CONFIG_UI_RADIUS,
					style->active_bg_c, style->highlight_c, style->shadow_c);
			uv_ft81x_draw_string("Space", style->font->index, LCD_W(0.5), y + BUTTON_H / 2,
					ALIGN_CENTER, style->text_color);
#endif
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
#if CONFIG_LCD
	uv_lcd_draw_rect(0, style->font->char_height,
			LCD_W(1), LCD_H(1 - KEYBOARD_HEIGHT) - 2 - style->font->char_height, style->window_c);

	// if the text is too long to fit to the screen,
	// replace the last space with a new line
	if (uv_ui_text_width_px(input, style->font, 1.0f) > LCD_W(1)) {
		for (int16_t i = strlen(input) - 1; i > 0 && input[i] != '\n'; i--) {
			if (input[i] == ' ') {
				input[i] = '\n';
				break;
			}
		}
	}
#endif

#if CONFIG_LCD
	_uv_ui_draw_text(0, style->font->char_height, style->font,
			ALIGN_TOP_LEFT, style->text_color, style->window_c, input, 1.0f);
#elif CONFIG_FT81X
	uv_ft81x_draw_string(input, style->font->index, LCD_WPPT(500), style->font->char_height,
			ALIGN_TOP_CENTER, style->text_color);
#endif

}



bool uv_uikeyboard_show(const char *title, char *buffer,
		uint16_t buf_len, const uv_uistyle_st *style) {

	uv_touch_st t;
	uint16_t input_len = 0;
	shift = false;
#if CONFIG_LCD
	bool pressed = uv_lcd_touch_get(&t.x, &t.y);
#elif CONFIG_FT81X
	bool pressed = uv_ft81x_get_touch(&t.x, &t.y);
#endif
	refresh = true;
	buffer[0] = '\0';

	while (true) {

#if CONFIG_LCD
		bool state = uv_lcd_touch_get(&t.x, &t.y);
#elif CONFIG_FT81X
		bool state = uv_ft81x_get_touch(&t.x, &t.y);
#endif
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
				if (input_len) input_len--;
				buffer[input_len] = '\0';
				update_input(buffer, style);
			}
			// normal character pressed
			else {
				if (input_len < buf_len - 1) {
					buffer[input_len++] = c;
					buffer[input_len] = '\0';
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
