/*
 * uv_label.c
 *
 *  Created on: Sep 30, 2016
 *      Author: usevolt
 */



#include "uv_ui.h"
#include <string.h>

#define this ((uv_label_st*)me)


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
static inline void draw_line(int16_t x, int16_t y, uint16_t width, const uv_font_st *font,
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
		x = x + width / 2 - len * font->char_width / 2;
	}
	else if (align & ALIGN_H_RIGHT) {
		x = x + width - len * font->char_width;
	}
	for (i = 0; i < len; i++) {
		draw_char(x, y, font, str[i], color);
		x += font->char_width;
	}
}


void uv_label_step(void *me, uint16_t step_ms) {
	// do nothing if refresh is not called
	// (label is a static object, it doesn't have any animations, etc.
	if (!this->super.refresh) {
		return;
	}

	uint16_t len = strlen(this->str);
	uint16_t line_count = 1;
	uint16_t i;
	int16_t y = 0;
	// calculate line count
	for (i = 0; i < len; i++) {
		if (this->str[i] == '\n') {
			line_count++;
		}
	}

	// calculate min height from line count and update the width of the label according to that
	uint16_t min_w = line_count * this->font->char_height;
	if (uv_ui_get_bb(me)->height < min_w) {
		uv_ui_get_bb(me)->height = min_w;
	}

	if (this->align & ALIGN_V_CENTER) {
		y = uv_ui_get_bb(me)->y + uv_ui_get_bb(me)->height / 2 - line_count * this->font->char_height / 2;
	}
	else if (this->align & ALIGN_V_BOTTOM) {
		y = uv_ui_get_bb(me)->y + uv_ui_get_bb(me)->height - line_count * this->font->char_height;
	}

	i = 0;
	while(this->str[i] != '\0') {
		while(this->str[i] == '\n' || this->str[i] == '\r') {
			i++;
		}
		draw_line(uv_ui_get_xglobal(me), uv_ui_get_yglobal(me) + y, uv_ui_get_bb(me)->width,
				this->font, &this->str[i], this->color, this->align);
		y += this->font->char_height;
		while(this->str[i] != '\n' && this->str[i] != '\r' && this->str[i] != '\0') {
			i++;
		}
	}
}


