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

#ifndef UV_HAL_INC_UI_UV_UILAYOUT_H_
#define UV_HAL_INC_UI_UV_UILAYOUT_H_

#include <uv_hal_config.h>
#include <uv_ui.h>


#if CONFIG_UI

/// @brief: Helper object which can be used to layout
/// objects in a grid layout
typedef struct {
	/// @brief: Grid x, y, width and height on the screen
	uv_bounding_box_st bb;
	/// @brief: Calculates the current running index of sequential calls to
	/// *next*-function
	int16_t index;
	/// @brief: x-axis cell count
	int16_t cell_count;
	/// @brief: y-axis row count
	int16_t row_count;
	/// @brief: Horizontal padding between cells
	int16_t hpadding;
	/// @brief: Vertical padding between rows
	int16_t vpadding;
	/// @brief: If true (default), the zells are returned in left to right, up to down order.
	/// Else, in up to down, left to right order.
	bool horizontal;
} uv_uigridlayout_st;


/// @brief: Initializes the grid layout
///
/// @param cell_count: Number of cells in x-axis
/// @param row_count: Number of rows in y-axis
void uv_uigridlayout_init(uv_uigridlayout_st *this, int16_t x, int16_t y,
		int16_t width, int16_t height, int16_t cell_count, int16_t row_count);

/// @brief: Sets the paddings
static inline void uv_uigridlayout_set_padding(uv_uigridlayout_st *this,
		int16_t horizontal_padding, int16_t vertical_padding) {
	this->hpadding = horizontal_padding;
	this->vpadding = vertical_padding;
}

/// @brief: Sets the gridlayout to vertical order, i.e. from up to down, left to right.
static inline void uv_uigridlayout_set_vertical_order(uv_uigridlayout_st *this) {
	this->horizontal = false;
}

static inline void uv_uigridlayout_set_horizontal_order(uv_uigridlayout_st *this) {
	this->horizontal = true;
}

/// @brief: Sets the gridlayout's height. If this is changed in the middle of
/// laying the elements with *uv_uigridlayout_next*, the calculation of the cells
/// updates for the future cells.
static inline void uv_uigridlayout_set_height(uv_uigridlayout_st *this, int16_t value) {
	this->bb.height = value;
}

/// @brief: Sets the gridlayout's width. If this is changed in the middle of
/// laying the elements with *uv_uigridlayout_next*, the calculation of the cells
/// updates for the future cells.
static inline void uv_uigridlayout_set_width(uv_uigridlayout_st *this, int16_t value) {
	this->bb.width = value;
}

/// @brief: Sets the horizontal row count. This can be called in the middle of calling
/// *uv_uigridlayout_next*
void uv_uigridlayout_set_row_count(uv_uigridlayout_st *this, int16_t value);


/// @brief: Returns the bounding box describing the position and dimensions
/// of the next entry in the grid.
///
/// @note: Grid entries go from left to right and from up to down -order.
uv_bounding_box_st uv_uigridlayout_next(uv_uigridlayout_st *this);



/// @brief: Defines a uistrlayout module. uistrlayout is a layout that defines
/// the cells with a string given to it. The string *str* _has_ to be null-terminated.
///
/// @info: The string defines all the cells in uistrlayout. Each cell should have a unique
/// name, horizontally separated with '|' and vertically with new line character '\n'.
/// White space is evaluated as a part of the cell names and the cell names are case-sensitive.
/// Additionally a span-character '#' can be used to determine how many horizontal
/// cells the cell fills. The '#' character should be followed with the amount of cells to fill.
/// This span-tag is not calculated being part of the cell name.
///
/// @example: "a|#2b|c\nd1|e" string constructs a layout:
///	a   b   b    c
///	  d1   e
/// Where each horizontal row is divided equally for the cells. Thus cells 'd' and 'e'
/// are bigger than 'a', 'b' and 'c', but because of cell spanning, b is double to a and c.
typedef struct {
	uv_bounding_box_st bb;
	int16_t h_padding;
	int16_t v_padding;
	const char *str;
	// calculated row count from the str in the init function
	uint8_t row_count;
	// The current index calculated with *uv_uistrlayout_next* or
	// *uv_uistrlayout_find*.
	uint8_t index;
	// When true, the uistrlayout lays the cells in left-to-right, up-to-down order.
	// when false, the uistrlayout lays the cells in up-to-down, left-to-right order.
	// Defaults to true.
	bool horizontal;
} uv_uistrlayout_st;


/// @brief: Initializes the uistrlayout
///
/// @param str: The sring which build up the layout. See uv_uistrlayout_st definition for help
/// @param bb: The bounding box of this layout.
/// @param h_padding: The horizontal padding between cells in pixels
/// @param v_padding: The vertical padding between rows in pixels
void uv_uistrlayout_init(uv_uistrlayout_st *this, const char *str,
		int16_t x, int16_t y, int16_t width, int16_t height,
		int16_t h_padding, int16_t v_padding);


/// @brief: Sets the layout in down-to-up, left-to-right laying order.
///
/// @note: Additional way of marking the layout vertical is to prefix the layout string with
/// *#V# characters in uv_uistrlayout_init function.
static inline void uv_uistrlayout_set_vertical(uv_uistrlayout_st *this) {
	this->horizontal = false;
}


/// @brief: Returns the bounding box of the "next" cell. This shouldn't be called
/// more times than the string given to uistrlayout defines cells.
uv_bounding_box_st uv_uistrlayout_next(uv_uistrlayout_st *this);


/// @brief: Finds the cell with a name of "c" and returns the bounding box to it.
/// If multiple cells with the same name are defined, this returns the bounding box
/// to the first one.
uv_bounding_box_st uv_uistrlayout_find(uv_uistrlayout_st *this, const char *c);


/// @brief: Finds the next cell with a name of *c* starting from the last cell that was
/// returned.
uv_bounding_box_st uv_uistrlayout_find_next(uv_uistrlayout_st *this, const char *c);


/// @brief: Returns true if a cell with a name of *c* can be found from the layout.
bool uv_uistrlayout_contains(uv_uistrlayout_st *this, const char *c);

#endif

#endif /* UV_HAL_INC_UI_UV_UILAYOUT_H_ */
