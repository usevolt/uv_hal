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
#include <string.h>

#if CONFIG_UI


uint32_t uv_ui_utf8_next(const char **str_ptr) {
	const uint8_t *s = (const uint8_t *) *str_ptr;
	uint32_t cp;
	uint8_t consumed;
	uint8_t c = s[0];
	if (c < 0x80u) {
		// plain ASCII
		cp = c;
		consumed = 1;
	}
	else if (((c & 0xE0u) == 0xC0u) && ((s[1] & 0xC0u) == 0x80u)) {
		// two-byte sequence
		cp = ((uint32_t) (c & 0x1Fu) << 6) | (uint32_t) (s[1] & 0x3Fu);
		consumed = 2;
	}
	else if (((c & 0xF0u) == 0xE0u) && ((s[1] & 0xC0u) == 0x80u) &&
			((s[2] & 0xC0u) == 0x80u)) {
		// three-byte sequence
		cp = ((uint32_t) (c & 0x0Fu) << 12) |
				((uint32_t) (s[1] & 0x3Fu) << 6) |
				(uint32_t) (s[2] & 0x3Fu);
		consumed = 3;
	}
	else if (((c & 0xF8u) == 0xF0u) && ((s[1] & 0xC0u) == 0x80u) &&
			((s[2] & 0xC0u) == 0x80u) && ((s[3] & 0xC0u) == 0x80u)) {
		// four-byte sequence
		cp = ((uint32_t) (c & 0x07u) << 18) |
				((uint32_t) (s[1] & 0x3Fu) << 12) |
				((uint32_t) (s[2] & 0x3Fu) << 6) |
				(uint32_t) (s[3] & 0x3Fu);
		consumed = 4;
	}
	else {
		// invalid / truncated lead byte: consume a single byte
		cp = c;
		consumed = 1;
	}
	*str_ptr = (const char *) (s + consumed);
	return cp;
}


uint8_t uv_ui_codepoint_glyph(uint32_t codepoint, bool nordic_glyphs) {
	uint8_t ret;
	if (codepoint < 0x80u) {
		ret = (uint8_t) codepoint;
	}
	else if (codepoint == 0x00E4u) {
		// ä -> custom glyph slot, or fall back to 'a'
		ret = nordic_glyphs ? UV_UI_GLYPH_a_UML : (uint8_t) 'a';
	}
	else if (codepoint == 0x00F6u) {
		// ö -> 'o'
		ret = nordic_glyphs ? UV_UI_GLYPH_o_UML : (uint8_t) 'o';
	}
	else if (codepoint == 0x00E5u) {
		// å -> 'a'
		ret = nordic_glyphs ? UV_UI_GLYPH_a_RING : (uint8_t) 'a';
	}
	else if (codepoint == 0x00C4u) {
		// Ä -> 'A'
		ret = nordic_glyphs ? UV_UI_GLYPH_A_UML : (uint8_t) 'A';
	}
	else if (codepoint == 0x00D6u) {
		// Ö -> 'O'
		ret = nordic_glyphs ? UV_UI_GLYPH_O_UML : (uint8_t) 'O';
	}
	else if (codepoint == 0x00C5u) {
		// Å -> 'A'
		ret = nordic_glyphs ? UV_UI_GLYPH_A_RING : (uint8_t) 'A';
	}
	else {
		// no glyph available for this codepoint
		ret = (uint8_t) '?';
	}
	return ret;
}


uint16_t uv_ui_str_to_glyphs(const char *src, char *dst, uint16_t dst_len,
		bool nordic_glyphs) {
	uint16_t n = 0;
	if ((src != NULL) && (dst != NULL) && (dst_len > 0)) {
		const char *p = src;
		while ((*p != '\0') && (n < (dst_len - 1))) {
			uint32_t cp = uv_ui_utf8_next(&p);
			dst[n] = (char) uv_ui_codepoint_glyph(cp, nordic_glyphs);
			n++;
		}
		dst[n] = '\0';
	}
	return n;
}



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


color_t uv_uic_alpha(color_t c, int16_t value) {
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


int16_t uv_ui_get_string_height(char *str, ui_font_st *font) {

	int16_t ret = uv_ui_get_font_height(font) * uv_str_get_line_count(str);

	return ret;
}

int16_t uv_str_get_line_count(const char *str) {
	int16_t ret = 0;
	if (str) {
		ret = (*str == '\0') ? 0 : 1;
		while (*str != '\0') {
			if (*str++ == '\n') {
				ret++;
			}
		}
	}
	return ret;
}



const char *uv_str_next_line(const char *str) {
	if (str != NULL) {
		while (*str != '\0' &&
				*str != '\n') {
			str++;
		}
		if (*str == '\n') {
			str++;
		}
	}
	return str;
}



// --- public drawing / touch wrappers ----------------------------------------
//
// Each public uv_ui_* primitive is a thin wrapper: it calls the active backend's
// *_impl (uv_ft81x.c or uv_ui_opengl.c / uv_ui_x11.c) and, when CONFIG_UI_REMOTE
// is enabled, tees the call into the remote UI encoder so the identical frame
// can be rebuilt on a remote device. Keeping the tee here (backend-neutral)
// means the encode logic is single-sourced and every widget keeps calling the
// unchanged public API. The composite uv_ui_draw_shadow* helpers above are
// mirrored for free since they decompose into these public primitives.

void uv_ui_clear(color_t c) {
	uv_ui_clear_impl(c);
#if CONFIG_UI_REMOTE
	uv_ui_remote_encode_clear(c);
#endif
}

void uv_ui_dlswap(void) {
	uv_ui_dlswap_impl();
#if CONFIG_UI_REMOTE
	uv_ui_remote_encode_frame_end();
#endif
}

void uv_ui_draw_bitmap_ext(uv_uimedia_st *bitmap, int16_t x, int16_t y,
		int16_t w, int16_t h, uint32_t wrap, color_t c) {
	uv_ui_draw_bitmap_ext_impl(bitmap, x, y, w, h, wrap, c);
#if CONFIG_UI_REMOTE
	uv_ui_remote_encode_bitmap(bitmap, x, y, w, h, wrap, c);
#endif
}

void uv_ui_draw_point(int16_t x, int16_t y, color_t color, uint16_t diameter) {
	uv_ui_draw_point_impl(x, y, color, diameter);
#if CONFIG_UI_REMOTE
	uv_ui_remote_encode_point(x, y, color, diameter);
#endif
}

void uv_ui_draw_rrect(const int16_t x, const int16_t y,
		const uint16_t width, const uint16_t height,
		const uint16_t radius, const color_t color) {
	uv_ui_draw_rrect_impl(x, y, width, height, radius, color);
#if CONFIG_UI_REMOTE
	uv_ui_remote_encode_rrect(x, y, width, height, radius, color);
#endif
}

void uv_ui_draw_line(const int16_t start_x, const int16_t start_y,
		const int16_t end_x, const int16_t end_y,
		const uint16_t width, const color_t color) {
	uv_ui_draw_line_impl(start_x, start_y, end_x, end_y, width, color);
#if CONFIG_UI_REMOTE
	uv_ui_remote_encode_line(start_x, start_y, end_x, end_y, width, color);
#endif
}

void uv_ui_draw_linestrip(const uv_ui_linestrip_point_st *points,
		const uint16_t point_count, const uint16_t line_width, const color_t color,
		const uv_ui_strip_type_e type) {
	uv_ui_draw_linestrip_impl(points, point_count, line_width, color, type);
#if CONFIG_UI_REMOTE
	uv_ui_remote_encode_linestrip(points, point_count, line_width, color, type);
#endif
}

void uv_ui_draw_polygon(const uv_ui_linestrip_point_st *points,
		const uint16_t point_count, const color_t color) {
	uv_ui_draw_polygon_impl(points, point_count, color);
#if CONFIG_UI_REMOTE
	uv_ui_remote_encode_polygon(points, point_count, color);
#endif
}

void uv_ui_draw_string(char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color) {
	uv_ui_draw_string_impl(str, font, x, y, align, color);
#if CONFIG_UI_REMOTE
	uv_ui_remote_encode_string(str, font, x, y, align, color);
#endif
}

void uv_ui_set_mask(int16_t x, int16_t y, int16_t width, int16_t height) {
	uv_ui_set_mask_impl(x, y, width, height);
#if CONFIG_UI_REMOTE
	uv_ui_remote_encode_mask(x, y, width, height);
#endif
}

bool uv_ui_get_touch(int16_t *x, int16_t *y) {
	bool ret;
#if CONFIG_UI_REMOTE
	int16_t rx = 0;
	int16_t ry = 0;
	if (uv_ui_remote_active() && uv_ui_remote_get_touch(&rx, &ry)) {
		// a connected remote overrides the local hardware touch this cycle
		if (x != NULL) {
			*x = rx;
		}
		if (y != NULL) {
			*y = ry;
		}
		ret = true;
	}
	else {
		ret = uv_ui_get_touch_impl(x, y);
	}
#else
	ret = uv_ui_get_touch_impl(x, y);
#endif
	return ret;
}


#endif
