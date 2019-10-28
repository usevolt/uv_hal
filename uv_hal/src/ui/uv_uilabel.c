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



#include "uv_ui.h"
#include "uv_utilities.h"

#if CONFIG_UI

#define this ((uv_uilabel_st*)me)

	void uv_uilabel_init(void *me, uv_font_st *font,
			alignment_e alignment, color_t color, char *str) {
	uv_uiobject_init(this);
	this->font = font;
	this->str = str;
	this->align = alignment;
	this->color = color;
	uv_uiobject_set_draw_callb(this, &_uv_uilabel_draw);
}





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
	uv_ft81x_draw_string(this->str, this->font, x, y, this->align, this->color);

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





#undef this
#define this ((uv_uidigit_st*)me)


/// @brief: Initializes the digit label.
	void uv_uidigit_init(void *me, uv_font_st *font,
			alignment_e alignment, color_t color, char *format, int value) {
		uv_uilabel_init(me, font, alignment, color, "");
	this->divider = 1;
	strcpy(this->format, format);
	// force redraw
	uv_uidigit_set_value(this, !value);
	uv_uidigit_set_value(this, value);
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




