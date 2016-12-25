/*
 * uv_uilayout.c
 *
 *  Created on: Dec 23, 2016
 *      Author: usevolt
 */


#include "ui/uv_uilayout.h"


#if CONFIG_LCD


void uv_uigridlayout_init(uv_uigridlayout_st *this, int16_t x, int16_t y,
		int16_t width, int16_t height, int16_t cell_count, int16_t row_count) {
	this->bb.x = x;
	this->bb.y = y;
	this->bb.width = width;
	this->bb.height = height;
	this->cell_count = cell_count;
	this->row_count = row_count;
	this->index = 0;
	this->hpadding = 0;
	this->vpadding = 0;
}


uv_bounding_box_st uv_uigridlayout_next(uv_uigridlayout_st *this) {
	uv_bounding_box_st bb = {};
	if (this->index >= this->cell_count * this->row_count) {
		return bb;
	}
	int16_t cell_i = this->index % this->cell_count;
	int16_t row_i = this->index / this->cell_count;

	bb.x = this->bb.x + cell_i * this->bb.width / this->cell_count + this->hpadding;
	bb.y = this->bb.y + row_i * this->bb.height / this->row_count + this->vpadding;
	bb.width = this->bb.width / this->cell_count - this->hpadding * 2;
	bb.height = this->bb.height / this->row_count - this->vpadding * 2;

	this->index++;
	return bb;
}


#endif
