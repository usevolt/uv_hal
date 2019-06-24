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


/// @brief: Returns the bounding box describing the position and dimensions
/// of the next entry in the grid.
///
/// @note: Grid entries go from left to right and from up to down -order.
uv_bounding_box_st uv_uigridlayout_next(uv_uigridlayout_st *this);

#endif

#endif /* UV_HAL_INC_UI_UV_UILAYOUT_H_ */
