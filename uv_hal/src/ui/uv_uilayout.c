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


#include "ui/uv_uilayout.h"


#if CONFIG_UI


void uv_uigridlayout_init(uv_uigridlayout_st *this, int16_t x, int16_t y,
		int16_t width, int16_t height, int16_t column_count, int16_t row_count) {
	this->bb.x = x;
	this->bb.y = y;
	this->bb.width = width;
	this->bb.height = height;
	this->cell_count = column_count;
	this->row_count = row_count;
	this->index = 0;
	this->hpadding = 0;
	this->vpadding = 0;
	this->horizontal = true;
}


uv_bounding_box_st uv_uigridlayout_next(uv_uigridlayout_st *this) {
	uv_bounding_box_st bb = {};
	if (this->index >= this->cell_count * this->row_count) {
		return bb;
	}
	int16_t cell_i;
	int16_t row_i;
	if (this->horizontal) {
		cell_i = this->index % this->cell_count;
		row_i = this->index / this->cell_count;
	}
	else {
		row_i = this->index % this->row_count;
		cell_i = this->index / this->row_count;
	}

	bb.x = this->bb.x + cell_i * this->bb.width / this->cell_count + this->hpadding;
	bb.y = this->bb.y + row_i * this->bb.height / this->row_count + this->vpadding;
	bb.width = this->bb.width / this->cell_count - this->hpadding * 2;
	bb.height = this->bb.height / this->row_count - this->vpadding * 2;

	this->index++;
	return bb;
}


#endif
