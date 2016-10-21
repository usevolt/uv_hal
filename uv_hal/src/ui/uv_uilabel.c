/*
 * uv_label.c
 *
 *  Created on: Sep 30, 2016
 *      Author: usevolt
 */



#include "uv_ui.h"
#include <string.h>

#define this ((uv_uilabel_st*)me)


static inline int get_bit(uint8_t *base, uint16_t i) {
	uint16_t byte = i / 8;
	uint16_t b = i % 8;
	return ((base[byte] & (1 << (b))) >> b);
}


/// @brief: Draws a single character on the screen
static inline void draw_char(int16_t x, int16_t y, const uv_font_st *font, char c, color_t color) {
	// NOTE: fonts store pixels as bits!
	uint16_t i, j, k = 0, cc;
	uint16_t char_len = (font->char_height * font->char_width / 8) +
			((font->char_height * font->char_width) % 8 != 0);
	cc = (uint8_t) c;
	// k is a index number
	for (j = 0; j < font->char_height; j++) {
		for (i = 0; i < font->char_width; i++) {
			if (get_bit((uint8_t*) &font->p[cc * char_len], k)) {
				uv_lcd_draw_pixel(x + i, y + j, color);
			}
			k++;
		}
	}

}


/// @brief: Draws one line of text
static inline void draw_line(int16_t x, int16_t y, const uv_font_st *font,
		char *str, color_t color, alignment_e align) {
	uint16_t len = 0, i;
	for (i = 0; i < strlen(str); i++) {
		if (str[i] == '\n' || str[i] == '\r') {
			break;
		}
		else {
			len++;
		}
	}
	if (align & ALIGN_H_CENTER) {
		x = x - len * font->char_width / 2;
	}
	else if (align & ALIGN_H_RIGHT) {
		x = x - len * font->char_width;
	}
	for (i = 0; i < len; i++) {
		draw_char(x, y, font, str[i], color);
		x += font->char_width;
	}
}


void uv_uilabel_step(void *me, uv_touch_st *touch, uint16_t step_ms) {
	// do nothing if refresh is not called
	// (label is a static object, it doesn't have any animations, etc.
	if (!this->super.refresh) {
		return;
	}

	uint16_t x = uv_ui_get_xglobal(this),
			y = uv_ui_get_yglobal(this);
	if (this->align & ALIGN_H_CENTER) {
		x += uv_ui_get_bb(this)->width / 2;
	}
	else if (this->align & ALIGN_H_RIGHT) {
		x += uv_ui_get_bb(this)->width;
	}
	if (this->align & ALIGN_V_CENTER) {
		y += uv_ui_get_bb(this)->height / 2;
	}
	else if (this->align & ALIGN_V_BOTTOM) {
		y += uv_ui_get_bb(this)->height;
	}

	_uv_ui_draw_text(x, y, this->font, this->align, this->color, this->str);

}


void _uv_ui_draw_text(uint16_t x, uint16_t y, const uv_font_st *font,
		alignment_e align, color_t color, char *str) {

	uint16_t len = strlen(str);
	uint16_t line_count = 1;
	uint16_t i;
	int16_t yy = 0;
	// calculate line count
	for (i = 0; i < len; i++) {
		if (str[i] == '\n') {
			line_count++;
		}
	}

	if (align & ALIGN_V_CENTER) {
		yy = y - line_count * font->char_height / 2;
	}
	else if (align & ALIGN_V_BOTTOM) {
		yy = y - line_count * font->char_height;
	}

	i = 0;
	while(str[i] != '\0') {
		while(str[i] == '\n' || str[i] == '\r') {
			i++;
		}
		draw_line(x, y + yy, font, &str[i], color, align);
		yy += font->char_height;
		while(str[i] != '\n' && str[i] != '\r' && str[i] != '\0') {
			i++;
		}
	}
}


