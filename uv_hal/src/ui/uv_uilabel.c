/*
 * uv_label.c
 *
 *  Created on: Sep 30, 2016
 *      Author: usevolt
 */



#include "uv_ui.h"
#include "uv_utilities.h"

#if CONFIG_UI

#define this ((uv_uilabel_st*)me)

#if CONFIG_LCD

static inline int get_bit(uint8_t *base, uint16_t i) {
	uint16_t byte = i / 8;
	uint16_t b = i % 8;
	return ((base[byte] & (1 << (b))) >> b);
}


/// @brief: Draws a single character on the screen
static inline void draw_char(int16_t x, int16_t y,
		const uv_font_st *font, char c, color_t color, color_t bgcolor,
		float scalef, const uv_bounding_box_st *maskbb) {
	// NOTE: fonts store pixels as bits!
	uint16_t i, j = 0;
	uint16_t char_width_bytes = ((font->char_width / 8) + ((font->char_width % 8) != 0));
	uint16_t char_len = char_width_bytes * font->char_height;
	int px, py;
	int ratio = 0x10000 / scalef + 1;
	int w = font->char_width * scalef;
	int h = font->char_height * scalef;

	if ((int8_t) c - font->index_offset < 0) {
		// something went wrong: Trying to render a character which is not part of this font
		return;
	}
	for (j = 0; j < h; j++) {
		py = (j * ratio) >> 16;
		// p is a reference to character's current row
		uint8_t *p = (uint8_t*) &font->p[(c - font->index_offset) * char_len + (int) py * char_width_bytes];

		for (i = 0; i < w; i++) {
			if (((x + i) > maskbb->x) &&
					((x + i) < (maskbb->x + maskbb->width)) &&
					((y + j) > maskbb->y) &&
					((y + j) < (maskbb->y + maskbb->height))) {

				px = (i * ratio) >> 16;
				if (get_bit(p, (int) px)) {
					uv_lcd_draw_pixel(x + i, y + j, color);
				}
				else if ((bgcolor & 0xFF000000) != 0xFF000000) {
					uv_lcd_draw_pixel(x + i, y + j, bgcolor);
				}
			}
		}
	}
}
#endif



#if CONFIG_LCD
void uv_uilabel_init(void *me, const uv_font_st *font,
		alignment_e alignment, color_t color, color_t bgcolor, char *str) {
#elif CONFIG_FT81X
	void uv_uilabel_init(void *me, const uv_font_st *font,
			alignment_e alignment, color_t color, char *str) {
#endif
	uv_uiobject_init(this);
	this->font = font;
	this->str = str;
	this->align = alignment;
	this->color = color;
#if CONFIG_LCD
	this->bg_color = bgcolor;
	this->scale = 1.0f;
#endif
	uv_uiobject_set_draw_callb(this, &_uv_uilabel_draw);
	uv_uiobject_set_step_callb(this, &uv_uilabel_step);
}



#if CONFIG_LCD
/// @brief: Draws one line of text
void draw_line(int16_t x, int16_t y, const uv_font_st *font,
		const char *str, const color_t color, const color_t bgcolor,
		const alignment_e align, const float scale, const uv_bounding_box_st *maskbb) {

	if ((y + font->char_height) < maskbb->y ||
			(y > (maskbb->y + maskbb->height)) ||
			(x > (maskbb->x + maskbb->width))) {
		return;
	}
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
		x = x - len * font->char_width * scale / 2;
	}
	else if (align & ALIGN_H_RIGHT) {
		x = x - len * font->char_width * scale;
	}
	for (i = 0; i < len; i++) {
		draw_char(x, y, font, str[i], color, bgcolor, scale, maskbb);
		x += font->char_width * scale;
	}
}
#endif


void _uv_uilabel_draw(void *me, const uv_bounding_box_st *pbb) {
	uint16_t x = uv_ui_get_xglobal(this),
			y = uv_ui_get_yglobal(this);

	if ((this->align == ALIGN_CENTER) ||
			(this->align == ALIGN_TOP_CENTER)) {
		x += uv_uibb(this)->width / 2;
	}
	else if ((this->align == ALIGN_CENTER_RIGHT) ||
			(this->align == ALIGN_TOP_RIGHT)) {
		x += uv_uibb(this)->width;
	}
	if ((this->align == ALIGN_CENTER) ||
			(this->align == ALIGN_CENTER_LEFT) ||
			(this->align == ALIGN_CENTER_RIGHT)) {
		y += uv_uibb(this)->height / 2;
	}
	uv_ft81x_draw_string(this->str, this->font->index, x, y, this->align, this->color);

}

uv_uiobject_ret_e uv_uilabel_step(void *me, uint16_t step_ms, const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (((uv_uiobject_st*) this)->refresh) {
		_uv_uiobject_draw(this, pbb);
	}

	return ret;
}



void uv_uilabel_set_text(void *me, char *str) {
	if (strcmp(this->str, str) != 0) {
		this->str = str;
		uv_ui_refresh(me);
	}
}


/// @brief: Sets the color of the label text
void uv_uilabel_set_color(void *me, color_t c) {
	if (this->color != c) {
		this->color = c;
		uv_ui_refresh(me);
	}
}




int16_t uv_ui_text_height_px(char *str, const uv_font_st *font, float scale) {
	int16_t line_count = 1;
	char *c = str;
	if (!str || *c == '\0') {
		return 0;
	}
	while (*c != '\0') {
		if (*c == '\n' || *c == '\r') {
			if (*(c+1) != '\n' && *(c+1) != '\r') {
				line_count++;
			}
		}
		c++;
	}
	return line_count * font->char_height * scale;
}


#undef this
#define this ((uv_uidigit_st*)me)


/// @brief: Initializes the digit label.
	void uv_uidigit_init(void *me, const uv_font_st *font,
			alignment_e alignment, color_t color, char *format, int value) {
		uv_uilabel_init(me, font, alignment, color, "");
	this->divider = 1;
	strcpy(this->format, format);
	// force redraw
	uv_uidigit_set_value(this, !value);
	uv_uidigit_set_value(this, value);
	((uv_uiobject_st*) this)->step_callb = &uv_uidigit_step;
}


void uv_uidigit_set_value(void *me, int value) {
	if (this->value != value) {
		uv_uilabel_set_text(me, "");
		this->value = value;
		int val = value / (this->divider);
		unsigned int cval = abs(value) % (this->divider);

		if (this->divider != 1) {
			sprintf(this->str, this->format, val, cval);
		}
		else {
			sprintf(this->str, this->format, val);
		}
		this->super.str = this->str;
		uv_ui_refresh(this);
	}
}


void uv_uidigit_set_text(void *me, char *str) {
	strcpy(this->str, str);
	uv_uilabel_set_text(this, str);
}




#endif




