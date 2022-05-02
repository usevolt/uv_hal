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
#ifndef HAL_UV_HAL_INC_UI_UV_UIGRAPH_H_
#define HAL_UV_HAL_INC_UI_UV_UIGRAPH_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_uiobject.h"

/// @file: defines a uigraph module. Uigraph is a graph that draws a graph background
/// and a graph specified by the given values.

#if CONFIG_UI


#ifndef CONFIG_UI_GRAPH_LINE_WIDTH
#error "CONFIG_UI_GRAPH_LINE_WIDTH should define the line width\
 used when drawing the graph in pixels"
#endif

typedef struct __attribute__((packed)) {
	int16_t x;
	int16_t y;
	bool interactive;
} uv_uigraph_point_st;


void uv_uigraph_point_init(uv_uigraph_point_st *this,
		int16_t x, int16_t y, bool interactive);



typedef enum {
	UIGRAPH_POINT_DIR_X_NEG = 0,
	UIGRAPH_POINT_DIR_X_POS,
	UIGRAPH_POINT_DIR_Y_NEG,
	UIGRAPH_POINT_DIR_Y_POS,
	UIGRAPH_POINT_DRAG
} uv_uigraph_point_dir_e;


typedef struct __attribute__((packed)) {
	EXTENDS(uv_uiobject_st);

	color_t coordinate_c;
	color_t graph_c;

	uv_uigraph_point_st *points;
	uint16_t points_count;
	int16_t active_point;
	bool point_selected;
	bool point_changed;
	int32_t min_x;
	int32_t min_y;
	int32_t max_x;
	int32_t max_y;
	int16_t current_val_x;
	int16_t current_val_y;
	bool (*point_moved_callb)(int16_t, int16_t, int16_t);
	uv_delay_st press_delay;
	int16_t drag_start_x;
	int16_t drag_start_y;
	int16_t drag_x;
	int16_t drag_y;
	uv_uigraph_point_dir_e point_dir;

	// helper variables that define the content width and height. These are
	// calculated in the draw function and stored here, so that they can be used
	// in touch function.
	int16_t content_w;
	int16_t content_x;
	int16_t content_h;
	char *title;
	const char *x_unit;
	const char *y_unit;
	const uv_uistyle_st *style;
} uv_uigraph_st;




#ifdef this
#undef this
#endif
#define this ((uv_uigraph_st*)me)



/// @brief: Initializes the button
///
/// @param points_buffer: Pointer to an array of *uv_uigraph_point_st* objects,
/// that should define the graph drawn on the screen
/// @param min_x: The minimum value for the X axis, i.e. the left edge
/// @param max_x: The maximum value for the X axis, i.e. the right edge
/// @param min_y: The minimum value for the Y axis, i.e. the bottom edge
/// @param max_y: The maximum value for the Y axis, i.e. the top edge
/// @param style: Pointer to the ui style used
void uv_uigraph_init(void *me, uv_uigraph_point_st *points_buffer,
		uint16_t points_count, int32_t min_x, int32_t max_x,
		int32_t min_y, int32_t max_y, const uv_uistyle_st *style);





/// @brief: Returns the index number of the selected point. If no points are
/// selected, returns -1.
static inline int16_t uv_uigraph_get_selected_point(void *me) {
	return this->active_point;
}


/// @brief: Sets the title text of the uigraph
static inline void uv_uigraph_set_title(void *me, char *str) {
	this->title = str;
}



/// @brief: Returns the title if assigned
static inline char *uv_uigraph_get_title(void *me) {
	return this->title;
}


static inline int32_t uv_uigraph_get_x_min(void *me) {
	return this->min_x;
}

static inline int32_t uv_uigraph_get_x_max(void *me) {
	return this->max_x;
}

static inline int32_t uv_uigraph_get_y_min(void *me) {
	return this->min_y;
}

static inline int32_t uv_uigraph_get_y_max(void *me) {
	return this->max_y;
}

static inline void uv_uigraph_set_min(void *me, int32_t x, int32_t y) {
	this->min_x = x;
	this->min_y = y;
	uv_ui_refresh(this);
}

static inline void uv_uigraph_set_max(void *me, int32_t x, int32_t y) {
	this->max_x = x;
	this->max_y = y;
	uv_ui_refresh(this);
}

/// @brief: Sets the coordinate color of the uigraph
static inline void uv_uigraph_set_coordinate_color(void *me, color_t c) {
	this->coordinate_c = c;
}

/// @brief: Returns the uigraph coordinate color
static inline color_t uv_uigraph_get_coordinate_color(void *me) {
	return this->coordinate_c;
}



/// @brief: Returns true for 1 step cycle when a point was selected
static inline bool uv_uigraph_point_selected(void *me) {
	return this->point_selected;
}


/// @brief: Returns true for 1 step cycle if the active point's value was changed
static inline bool uv_uigraph_point_value_changed(void *me) {
	return this->point_changed;
}

/// @brief: Sets the main color of the uibutton. The button should be refreshed after
/// calling this.
static inline void uv_uigraph_set_graph_color(void *me, color_t c) {
	this->graph_c = c;
}

/// @brief: Returns the button main color
static inline color_t uv_uigraph_get_graph_color(void *me) {
	return this->graph_c;
}



/// @brief: Sets the x axis unit
static inline void uv_uigraph_set_xunit(void *me, const char *str) {
	this->x_unit = str;
}


static inline const char *uv_uigraph_get_xunit(void *me) {
	return this->x_unit;
}

/// @brief: Sets the y axis unit
static inline void uv_uigraph_set_yunit(void *me, const char *str) {
	this->y_unit = str;
}

static inline const char *uv_uigraph_get_yunit(void *me) {
	return this->y_unit;
}


void uv_uigraph_set_point_count(void *me, uint16_t value);


static inline uint16_t uv_uigraph_get_point_count(void *me) {
	return this->points_count;
}


/// @brief: Sets the current value on the graph. The *val_x* is shown
/// as a vertical line, and *val_y* is shown as a horizontal line.
// To disable any one of these, enter a value smaller than the minimum or bigger than
/// the maximum.
void uv_uigraph_set_current_val(void *me, int16_t val_x, int16_t val_y);



/// @brief: Sets the uigraph as editable. Defaults to false. When editable,
/// Interactive points can be clicked and moved around like in uivalveslider.
///
/// @param *point_moved_callb callback function that has to return true if the
/// new coordinates for the *point* specified in *y_new* and *x_new* are accepted.
/// The point array is updated automatically if accepted.
/// If false is returned, the point is not moved.
static inline void uv_uigraph_set_editable(void *me,
		bool (*point_moved_callb)(int16_t point, int16_t x_new, int16_t y_new)) {
	this->point_moved_callb = point_moved_callb;
}



/// @brief: Step function should be called every step cycle
uv_uiobject_ret_e uv_uigraph_step(void *me, uint16_t step_ms);



/// @brief: Draw function. Normally this is called internally but it can also be
/// called when using draw callbacks
void uv_uigraph_draw(void *me, const uv_bounding_box_st *pbb);



void uv_uigraph_touch(void *me, uv_touch_st *touch);




#undef this




#endif
#endif /* HAL_UV_HAL_INC_UI_UV_UIGRAPH_H_ */
