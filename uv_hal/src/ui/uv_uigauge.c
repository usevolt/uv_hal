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
#include "ui/uv_uigauge.h"

#if CONFIG_UI

#include <stdio.h>


/// @brief: Quarter-wave sine lookup table, sin(deg) * 1024 for deg = 0 ... 90.
/// Used to place arc points without pulling in libm.
static const int16_t sin_q10[91] = {
	0, 18, 36, 54, 71, 89, 107, 125, 143, 160,
	178, 195, 213, 230, 248, 265, 282, 299, 316, 333,
	350, 367, 384, 400, 416, 433, 449, 465, 481, 496,
	512, 527, 543, 558, 573, 587, 602, 616, 630, 644,
	658, 672, 685, 698, 711, 724, 737, 749, 761, 773,
	784, 796, 807, 818, 828, 839, 849, 859, 868, 878,
	887, 896, 904, 912, 920, 928, 935, 943, 949, 956,
	962, 968, 974, 979, 984, 989, 994, 998, 1002, 1005,
	1008, 1011, 1014, 1016, 1018, 1020, 1022, 1023, 1023, 1024,
	1024
};


/// @brief: Returns sin(deg) * 1024 for any integer degree angle.
static int32_t isin(int32_t deg) {
	int32_t ret;
	deg %= 360;
	if (deg < 0) {
		deg += 360;
	}
	else {
		// MISRA 15.7
	}
	if (deg <= 90) {
		ret = sin_q10[deg];
	}
	else if (deg <= 180) {
		ret = sin_q10[180 - deg];
	}
	else if (deg <= 270) {
		ret = -sin_q10[deg - 180];
	}
	else {
		ret = -sin_q10[360 - deg];
	}
	return ret;
}


/// @brief: Returns cos(deg) * 1024 for any integer degree angle.
static int32_t icos(int32_t deg) {
	return isin(deg + 90);
}



#define this ((uv_uigauge_st*)me)



void uv_uigauge_init(void *me, int32_t min_val, int32_t max_val,
		color_t bg_c, const uv_uistyle_st *style) {
	uv_uiobject_init(this);
	this->min_val = min_val;
	this->max_val = max_val;
	this->value = min_val;
	this->thickness = 18;
	this->seg_step = 4;
	this->zones = NULL;
	this->zone_count = 0;
	this->title = NULL;
	this->unit = NULL;
	this->value_str[0] = '\0';
	this->style = style;
	this->value_font = style->font;
	// opaque grey track for the unfilled remainder
	this->arc_bg_c = C(0xFF505050);
	// the donut hole is painted in the background color so it reads as empty
	this->hole_c = bg_c;

	uv_uiobject_set_draw_callb(this, &uv_uigauge_draw);
}



/// @brief: Maps a value to its angle on the gauge, in degrees. 180 == left
/// (min), 90 == top, 0 == right (max).
static int16_t value_angle(const uv_uigauge_st *g, int32_t value) {
	int32_t rel = uv_reli(value, g->min_val, g->max_val);
	LIMITS(rel, 0, 1000);
	return (int16_t)(180 - (int32_t) 180 * rel / 1000);
}



/// @brief: Fills a pie wedge with its apex at the center (cx, cy) and its arc
/// on the outer radius *r*, spanning from *a_start* down to *a_end* degrees
/// (a_start >= a_end). Filled as a convex polygon, so both straight sides are
/// clean radial edges pointing at the center.
static void draw_wedge(int16_t cx, int16_t cy, int16_t r,
		int16_t a_start, int16_t a_end, color_t color) {
	uv_ui_linestrip_point_st pts[96];
	uint16_t n = 0;
	// apex first (also the triangle-fan center)
	pts[n].x = cx;
	pts[n].y = cy;
	n++;
	int16_t a = a_start;
	// arc points along the outer edge, from a_start towards a_end
	while ((a > a_end) && (n < (uint16_t) (sizeof(pts) / sizeof(pts[0]) - 1))) {
		pts[n].x = cx + r * icos(a) / 1024;
		pts[n].y = cy - r * isin(a) / 1024;
		n++;
		a -= 2;
	}
	// exact end of the arc
	pts[n].x = cx + r * icos(a_end) / 1024;
	pts[n].y = cy - r * isin(a_end) / 1024;
	n++;
	uv_ui_draw_polygon(pts, n, color);
}



void uv_uigauge_draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;

	int16_t title_h = (this->title == NULL) ? 0 :
			uv_ui_get_string_height((char*) this->title, this->style->font);

	// center of the half circle, near the bottom edge (leaving room for the title)
	int16_t cx = x + w / 2;
	int16_t cy = y + h - title_h - 4;
	// outer radius of the donut band
	int16_t r_out = MIN(w / 2, h - title_h) - 2;
	if (r_out < 2) {
		r_out = 2;
	}
	else {
		// MISRA 15.7
	}
	// inner radius (the donut hole)
	int16_t r_in = r_out - this->thickness;
	if (r_in < 1) {
		r_in = 1;
	}
	else {
		// MISRA 15.7
	}

	// The gauge is drawn as filled pie wedges (one for the grey track, one per
	// colored zone) and then the middle is punched out with a background-colored
	// disc to form the donut hole. A scissor clips everything below the cy
	// baseline so the hole disc keeps a clean flat bottom edge.
	uv_ui_set_mask(cx - r_out, cy - r_out, 2 * r_out, r_out + 1);

	// 1) grey track: full half-disc behind everything
	draw_wedge(cx, cy, r_out, 180, 0, this->arc_bg_c);

	// 2) one wedge per colored zone, each spanning from the zone's start angle
	//    to its end angle (clamped to the current value). The wedges tile the
	//    filled part of the arc, so every color boundary is a shared radial edge
	//    pointing at the center.
	if (this->zone_count > 0) {
		for (int16_t z = 0; z < (int16_t) this->zone_count; z++) {
			if (this->value > this->zones[z].start) {
				int32_t zend = ((z + 1) < (int16_t) this->zone_count) ?
						this->zones[z + 1].start : this->max_val;
				int16_t a_start = value_angle(this, this->zones[z].start);
				int16_t a_end = value_angle(this, MIN(this->value, zend));
				draw_wedge(cx, cy, r_out, a_start, a_end, this->zones[z].color);
			}
			else {
				// MISRA 15.7
			}
		}
	}
	else {
		// no zones configured: a single foreground-colored fill up to the value
		draw_wedge(cx, cy, r_out, 180, value_angle(this, this->value),
				this->style->fg_c);
	}

	// 3) punch the donut hole with a background-colored disc on top. Its lower
	//    half (below cy) is stripped by the active scissor.
	uv_ui_draw_point(cx, cy, this->hole_c, 2 * r_in);

	// restore the clip to the whole widget so the value and the title (which
	// sits below the cy baseline) are not clipped
	uv_ui_set_mask(x, y, w, h);

	// value drawn in the middle, on top of the gauge
	if (this->unit != NULL) {
		snprintf(this->value_str, sizeof(this->value_str), "%i %s",
				(int) this->value, this->unit);
	}
	else {
		snprintf(this->value_str, sizeof(this->value_str), "%i",
				(int) this->value);
	}
	// vertically center the value in the donut hole (between the diameter at cy
	// and the inner edge of the band at the top)
	int16_t value_y = cy - r_in / 2;
	uv_ui_draw_string(this->value_str, this->value_font,
			cx, value_y, ALIGN_CENTER, this->style->text_color);

	// 4) title below the gauge
	if (this->title != NULL) {
		uv_ui_draw_string((char*) this->title, this->style->font,
				cx, cy + 2, ALIGN_TOP_CENTER, this->style->text_color);
	}
	else {
		// MISRA 15.7
	}
}



void uv_uigauge_set_value(void *me, int32_t value) {
	if (this->value != value) {
		this->value = value;
		uv_ui_refresh(this);
	}
	else {
		// MISRA 15.7
	}
}



void uv_uigauge_set_zones(void *me,
		const uv_uigauge_zone_st *zones, uint8_t count) {
	this->zones = zones;
	this->zone_count = MIN(count, UIGAUGE_ZONE_COUNT_MAX);
	uv_ui_refresh(this);
}



void uv_uigauge_set_title(void *me, const char *title) {
	this->title = title;
	uv_ui_refresh(this);
}



void uv_uigauge_set_unit(void *me, const char *unit) {
	this->unit = unit;
	uv_ui_refresh(this);
}



void uv_uigauge_set_value_font(void *me, ui_font_st *font) {
	this->value_font = font;
	uv_ui_refresh(this);
}



void uv_uigauge_set_thickness(void *me, uint8_t thickness) {
	this->thickness = thickness;
	uv_ui_refresh(this);
}



void uv_uigauge_set_hole_color(void *me, color_t color) {
	this->hole_c = color;
	uv_ui_refresh(this);
}


#endif
