/*
 * uv_uigrid.h
 *
 *  Created on: Oct 30, 2016
 *      Author: usevolt
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


/// @brief: Returns the bounding box describing the position and dimensions
/// of the next entry in the grid.
///
/// @note: Grid entries go from left to right and from up to down -order.
uv_bounding_box_st uv_uigridlayout_next(uv_uigridlayout_st *this);

#endif

#endif /* UV_HAL_INC_UI_UV_UILAYOUT_H_ */
