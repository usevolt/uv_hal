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
#ifndef HAL_UV_HAL_INC_UI_UV_UIGAUGE_H_
#define HAL_UV_HAL_INC_UI_UV_UIGAUGE_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_uiobject.h"

/// @file: defines a uigauge module. Uigauge draws a half-circle "donut" gauge
/// (like a rainbow arc). The filled arc grows from the left (min value, a small
/// sector) towards the right (max value, full half circle). There is no needle;
/// the arc color is taken from the configured value zones. The current value is
/// drawn in the middle on top of the gauge and the title below it.

#if CONFIG_UI


/// @brief: Maximum number of color zones a single gauge can have
#define UIGAUGE_ZONE_COUNT_MAX		8


/// @brief: A single color zone. The zone applies to all values greater than or
/// equal to *start*, until the next zone's start. Zones should be given in
/// ascending *start* order.
typedef struct {
	int32_t start;
	color_t color;
} uv_uigauge_zone_st;



typedef struct {
	EXTENDS(uv_uiobject_st);

	int32_t min_val;
	int32_t max_val;
	int32_t value;

	// width of the donut band in pixels
	uint8_t thickness;
	// arc resolution in degrees per drawn segment (smaller == smoother)
	uint8_t seg_step;

	const uv_uigauge_zone_st *zones;
	uint8_t zone_count;

	const char *title;
	const char *unit;
	char value_str[12];

	const uv_uistyle_st *style;
	// font used for the value drawn in the middle. Defaults to style->font.
	ui_font_st *value_font;
	// color of the unfilled remainder of the arc (the track)
	color_t arc_bg_c;
	// color of the donut hole, i.e. the background showing through the middle.
	// Defaults to style->window_c.
	color_t hole_c;
} uv_uigauge_st;



#ifdef this
#undef this
#endif
#define this ((uv_uigauge_st*)me)



/// @brief: Initializes the gauge.
///
/// @param min_val: The value at the very left end of the arc (empty)
/// @param max_val: The value at the very right end of the arc (full half circle)
/// @param bg_c: Color of the donut hole. Should match the background that the
/// gauge is drawn on (e.g. the uidisplay color), so the hole reads as empty.
/// @param style: Pointer to the ui style used
void uv_uigauge_init(void *me, int32_t min_val, int32_t max_val,
		color_t bg_c, const uv_uistyle_st *style);


/// @brief: Sets the current value shown by the gauge. Refreshes the gauge only
/// when the value actually changed.
void uv_uigauge_set_value(void *me, int32_t value);

static inline int32_t uv_uigauge_get_value(void *me) {
	return this->value;
}


/// @brief: Sets the color zones used to paint the filled arc. The buffer must
/// stay valid for the lifetime of the gauge (typically a static const array).
void uv_uigauge_set_zones(void *me,
		const uv_uigauge_zone_st *zones, uint8_t count);


/// @brief: Sets the title drawn below the gauge. NULL disables it.
void uv_uigauge_set_title(void *me, const char *title);


/// @brief: Sets the unit string appended after the value (e.g. "1/min").
/// NULL disables it.
void uv_uigauge_set_unit(void *me, const char *unit);


/// @brief: Sets the font used for the value drawn in the middle of the gauge.
void uv_uigauge_set_value_font(void *me, ui_font_st *font);


/// @brief: Sets the donut band thickness in pixels.
void uv_uigauge_set_thickness(void *me, uint8_t thickness);


/// @brief: Sets the color of the donut hole (should match the background that
/// the gauge is drawn on).
void uv_uigauge_set_hole_color(void *me, color_t color);


void uv_uigauge_draw(void *me, const uv_bounding_box_st *pbb);



#undef this

#endif

#endif /* HAL_UV_HAL_INC_UI_UV_UIGAUGE_H_ */
