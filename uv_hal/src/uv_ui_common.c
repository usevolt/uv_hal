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


#include "uv_ui_common.h"

#if CONFIG_UI


static struct {
	int8_t grayscale_luminosity;
	ui_color_modes_e color_mode;
} ui_common = {
		.grayscale_luminosity = 0,
		.color_mode = COLOR_MODE_RGB

};
#ifdef this
#undef this
#endif
#define this (&ui_common)


color_t uv_uic_brighten(color_t c, int8_t value) {
	color_t ret = (c & 0xFF000000);
	for (uint8_t i = 0; i < 3; i++) {
		int32_t col = (c >> (i * 8)) & 0xFF;
		col += value;
		if (col < 0) {
			col = 0;
		}
		else if (col > 0xFF) {
			col = 0xFF;
		}
		else {

		}
		ret += (col << (i * 8));
	}
	return ret;
}


color_t uv_uic_alpha(color_t c, int8_t value) {
	color_t ret = c & ~(0xFF000000);
	int16_t alpha = (c & 0xFF000000) >> 24;
	alpha += value;
	LIMITS(alpha, 0, 0xFF);
	ret += (alpha << 24);

	return ret;
}


color_t uv_uic_grayscale(color_t c) {
	color_st *cst = (void*) &c;
	uint32_t shade = ((uint32_t) cst->r + cst->g + cst->b) / 3;
	if (shade > UINT8_MAX) {
		shade = UINT8_MAX;
	}
	if (this->grayscale_luminosity) {
		int32_t reli = uv_reli(this->grayscale_luminosity, INT8_MIN + 1, INT8_MAX);
		LIMITS(reli, 0, 1000);
		shade = shade * reli / 500;
	}
	cst->r = shade;
	cst->g = shade;
	cst->b = shade;
	memcpy(&c, cst, sizeof(c));

	return c;
}


color_st uv_uic(color_t c) {
	color_st ret;
	if (this->color_mode == COLOR_MODE_GRAYSCALE) {
		c = uv_uic_grayscale(c);
	}
	ret = *((color_st*) &c);

	return ret;
}



color_t uv_uic_lerpi(int32_t t, color_t ca, color_t cb) {
	uint32_t ret = 0;
	for (uint8_t i = 0; i < 4; i++) {
		int16_t val, start_val, end_val;
		start_val = (ca >> (i * 8)) & 0xFF;
		end_val = (cb >> (i * 8)) & 0xFF;

		val = uv_lerpi(t, start_val, end_val);
		ret += ((val & 0xFF) << (i * 8));
	}
	return (color_t) ret;
}



bool uv_ui_is_visible(const int16_t x, const int16_t y,
		const int16_t width, const int16_t height) {
	bool ret = true;
	if (((x + width) < 0) ||
			(x > CONFIG_FT81X_HSIZE) ||
			((y + height) < 0) ||
			(y > CONFIG_FT81X_VSIZE)) {
		return false;
	}

	return ret;
}


void uv_ui_set_grayscale_luminosity(int8_t value) {
	this->grayscale_luminosity = value;
}



void uv_ui_draw_shadowpoint(int16_t x, int16_t y,
		color_t color, color_t highlight_c, color_t shadow_c, uint16_t diameter) {
	uv_ui_draw_point(x - 2, y - 2, shadow_c, diameter);
	uv_ui_draw_point(x + 2, y + 2, highlight_c, diameter);
	uv_ui_draw_point(x, y, color, diameter);
}



void uv_ui_draw_shadowrrect(const int16_t x, const int16_t y,
		const uint16_t width, const uint16_t height,
		const uint16_t radius, const color_t color,
		const color_t highlight_c, const color_t shadow_c) {
	uv_ui_draw_rrect(x, y, width - 4, height - 4, radius, shadow_c);
	uv_ui_draw_rrect(x + 4, y + 4, width - 4, height - 4, radius, highlight_c);
	uv_ui_draw_rrect(x + 2, y + 2, width - 4, height - 4, radius, color);
}


void uv_ui_set_color_mode(ui_color_modes_e value) {
	this->color_mode = value;
}



ui_color_modes_e uv_ui_get_color_mode(void) {
	return this->color_mode;
}


#endif
