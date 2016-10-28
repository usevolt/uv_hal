/*
 * uv_uikeyboard.c
 *
 *  Created on: Oct 27, 2016
 *      Author: usevolt
 */

#include "ui/uv_uikeyboard.h"
#include <string.h>
#include "uv_rtos.h"

#if CONFIG_LCD



/// @brief: Lists all characters shown on the keyboard
/// *space* is the only special character which is shown below these
static const char letters[] = "1234567890qwertyuiopasdfghjklzxcvbnm";
static const char shift_letters[] = "1234567890QWERTYUIOPASDFGHJKLZXCVBNM";
static bool shift = false;

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


static void draw(const char *title, char *buffer, uint16_t buf_len, const uv_uikeyboard_style_st *style) {
	// fill whole screen
	uv_lcd_draw_rect(0, 0, LCD_W_PX, LCD_H_PX, style->bg_color);

	// draw title text
	_uv_ui_draw_text(LCD_W(0.5), 0, style->title_font, ALIGN_TOP_CENTER,
			style->text_color, style->bg_color, (char*) title);

	// draw buttons
	// half of the display is used to display the keyboard
	uint8_t line = 0;
	uint8_t line_counter = 0;
	int16_t x = 0;
	int16_t y = LCD_H(1 - KEYBOARD_HEIGHT) - 1;
	char str[2];
	for (int16_t i = 0; i < strlen(letters); i++) {
		uv_lcd_draw_rect(x, y, BUTTON_W, BUTTON_H, style->key_color);
		uv_lcd_draw_frame(x, y, BUTTON_W + 1, BUTTON_H + 1, 1, style->frame_color);
		str[0] = shift ? shift_letters[i] : letters[i];
		str[1] = '\0';
		_uv_ui_draw_text(x + BUTTON_W / 2, y + BUTTON_H / 2, style->text_font, ALIGN_CENTER,
				style->text_color, style->key_color, str);
		x += BUTTON_W;

		line_counter++;

		// draw backspace
		if (line == 0 && line_counter >= line_lengths[line]) {
			uv_lcd_draw_rect(x, y, BUTTON_W * 2, BUTTON_H, style->key_color);
			uv_lcd_draw_frame(x, y, BUTTON_W * 2, BUTTON_H + 1, 1, style->frame_color);
			_uv_ui_draw_text(x + BUTTON_W, y + BUTTON_H / 2, style->text_font, ALIGN_CENTER,
					style->text_color, style->key_color, "Backspace");
		}
		else if (line == 1 && line_counter >= line_lengths[line]) {
			// draw enter
			uv_lcd_draw_rect(x, y, BUTTON_W * 1.5, BUTTON_H * 2 + 1, style->key_color);
			uv_lcd_draw_frame(x, y, BUTTON_W * 1.5, BUTTON_H * 2 + 1, 1, style->frame_color);
			uv_lcd_draw_rect(x - BUTTON_W / 2 + 2, y + BUTTON_H + 1, BUTTON_W / 2, BUTTON_H - 1, style->key_color);
			_uv_ui_draw_text(x + BUTTON_W * 0.75, y + BUTTON_H, style->text_font, ALIGN_CENTER,
					style->text_color, style->key_color, "Enter");
		}
		// draw shift
		else if (line == 3 && line_counter >= line_lengths[line]) {
			uv_lcd_draw_rect(x, y, BUTTON_W * 2, BUTTON_H, style->key_color);
			uv_lcd_draw_frame(x, y, BUTTON_W * 2, BUTTON_H + 1, 1, style->frame_color);
			_uv_ui_draw_text(x + BUTTON_W, y + BUTTON_H / 2, style->text_font, ALIGN_CENTER,
					style->text_color, style->key_color, "Shift");
		}

		if (line_counter >= line_lengths[line]) {
			line_counter = 0;
			line++;
			x = BUTTON_W / 2 * line;
			y += BUTTON_H;
		}
	}
	// draw space bar
	uv_lcd_draw_rect(LCD_W(0.1), y, LCD_W(0.8), BUTTON_H, style->key_color);
	uv_lcd_draw_frame(LCD_W(0.1), y, LCD_W(0.8), BUTTON_H, 1, style->frame_color);
	_uv_ui_draw_text(LCD_W(0.5), y + BUTTON_H / 2, style->text_font, ALIGN_CENTER,
			style->text_color, style->key_color, "Space");
}



bool uv_uikeyboard_show(const char *title, char *buffer, uint16_t buf_len, const uv_uikeyboard_style_st *style) {

	draw(title, buffer, buf_len, style);

	while (true) {
		uv_rtos_task_yield();
	}

}





#endif
