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





/// @brief: True when *str* contains an ANSI escape (CSI) sequence, meaning it
/// carries inline colour information that draw_ansi_colored() must interpret.
static bool str_has_ansi(const char *str) {
	bool ret = false;
	if (str != NULL) {
		for (uint32_t i = 0; str[i] != '\0'; i++) {
			if (str[i] == 0x1B) {
				ret = true;
				break;
			}
		}
	}
	return ret;
}


/// @brief: Maps an ANSI SGR foreground code (30..37 normal, 90..97 bright) to a
/// colour. Code 0 resets to *deflt* (the label's own colour); unknown codes
/// (e.g. the bold attribute 1) leave the current colour *cur* unchanged.
static color_t ansi_sgr_color(int code, color_t cur, color_t deflt) {
	color_t ret = cur;
	switch (code) {
	case 0:  ret = deflt; break;
	case 30: case 90: ret = C(0xFF505050); break; // black (lifted so it is visible)
	case 31: case 91: ret = C(0xFFE02020); break; // red
	case 32: case 92: ret = C(0xFF30B030); break; // green
	case 33: case 93: ret = C(0xFFF2C200); break; // yellow
	case 34: case 94: ret = C(0xFF4878E0); break; // blue
	case 35: case 95: ret = C(0xFFC040C0); break; // magenta
	case 36: case 96: ret = C(0xFF20B0B0); break; // cyan
	case 37: case 97: ret = C(0xFFE8E8E8); break; // white
	default: ret = cur; break;
	}
	return ret;
}


/// @brief: Draws *str* left-aligned starting at (x0, y0), interpreting embedded
/// ANSI CSI colour sequences ("\033[...m") so coloured terminal output renders
/// coloured. Newlines start a new line; any non-colour escape is skipped.
static void draw_ansi_colored(uv_uilabel_st *label, int16_t x0, int16_t y0) {
	const char *s = label->str;
	color_t deflt = label->color;
	color_t cur = deflt;
	int16_t penx = x0;
	int16_t peny = y0;
	char run[256];
	int runlen = 0;

	for (uint32_t i = 0; s[i] != '\0'; i++) {
		char c = s[i];
		if ((c == 0x1B) || (c == '\n') || (c == '\r')) {
			// flush the accumulated run with the current colour
			if (runlen > 0) {
				run[runlen] = '\0';
				uv_ui_draw_string(run, label->font, penx, peny,
						ALIGN_TOP_LEFT, cur);
				penx += uv_ui_get_string_width(run, label->font);
				runlen = 0;
			}
			if (c == 0x1B) {
				// CSI sequence: "\033[" codes (';'-separated) terminated by a letter
				if (s[i + 1] == '[') {
					i += 2;
					while ((s[i] != '\0') && (s[i] != 'm') &&
							!((s[i] >= '@') && (s[i] <= '~') && (s[i] != ';'))) {
						int code = 0;
						bool has = false;
						while ((s[i] >= '0') && (s[i] <= '9')) {
							code = (code * 10) + (s[i] - '0');
							has = true;
							i++;
						}
						if (has) {
							cur = ansi_sgr_color(code, cur, deflt);
						}
						if (s[i] == ';') {
							i++;
						}
						else {
							break;
						}
					}
					// leave i on the terminating letter; the for-loop i++ skips it
				}
			}
			else if (c == '\n') {
				penx = x0;
				peny += label->font->char_height;
			}
			else {
				// carriage return: back to the line start, same row
				penx = x0;
			}
		}
		else {
			if (runlen >= (int) sizeof(run) - 1) {
				run[runlen] = '\0';
				uv_ui_draw_string(run, label->font, penx, peny,
						ALIGN_TOP_LEFT, cur);
				penx += uv_ui_get_string_width(run, label->font);
				runlen = 0;
			}
			run[runlen++] = c;
		}
	}
	if (runlen > 0) {
		run[runlen] = '\0';
		uv_ui_draw_string(run, label->font, penx, peny, ALIGN_TOP_LEFT, cur);
	}
}


void _uv_uilabel_draw(void *me, const uv_bounding_box_st *pbb) {
	uint16_t x = uv_ui_get_xglobal(this),
			y = uv_ui_get_yglobal(this);

	// labels carrying ANSI colour codes are drawn left-aligned with per-segment
	// colours; all other labels use the plain single-colour path unchanged
	if (str_has_ansi(this->str)) {
		// honour vertical centring (e.g. the one-line log strip) by lifting the
		// pen half a line; horizontal alignment stays left for the coloured flow
		if ((this->align == ALIGN_CENTER) ||
				(this->align == ALIGN_CENTER_LEFT) ||
				(this->align == ALIGN_CENTER_RIGHT)) {
			y += (uv_uibb(this)->height - this->font->char_height) / 2;
		}
		draw_ansi_colored(this, x, y);
		return;
	}

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
	uv_ui_draw_string(this->str, this->font, x, y, this->align, this->color);
}



void uv_uilabel_set_text(void *me, char *str) {
	if (this->str != NULL &&
			str != NULL) {
		if (strcmp(this->str, str) != 0) {
			uv_ui_refresh(me);
		}
	}
	this->str = str;
}


void uv_uilabel_set_color(void *me, color_t c) {
	if (this->color != c) {
		this->color = c;
		uv_ui_refresh(me);
	}
}





#undef this
#define this ((uv_uidigit_st*)me)


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




