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






void uv_uistrlayout_init(uv_uistrlayout_st *this, const char *str,
		int16_t x, int16_t y, int16_t width, int16_t height,
		int16_t h_padding, int16_t v_padding) {
	this->str = str;
	this->bb.x = x;
	this->bb.y = y;
	this->bb.width = width;
	this->bb.height = height;
	this->h_padding = h_padding;
	this->v_padding = v_padding;
	this->index = 0;
	this->horizontal = true;
	// special vertical marking in the string. Mark the layout as vertical.
	if (strncmp(str, "#V#", 3) == 0) {
		this->horizontal = false;
		this->str = str + 3;
	}
	// special horizontal marking in the string. Mark the layout as horizontal.
	else if (strncmp(str, "#H#", 3) == 0) {
		this->horizontal = true;
		this->str = str + 3;
	}
	else {

	}
	this->row_count = 1;
	for (uint16_t i = 0; i < strlen(this->str); i++) {
		if (this->str[i] == '\n') {
			this->row_count++;
		}
	}
}

void uv_uistrlayout_set_margin(uv_uistrlayout_st *this,
		int16_t margin_x, int16_t margin_y) {
	this->bb.x += margin_x;
	this->bb.w -= margin_x * 2;
	this->bb.y += margin_y;
	this->bb.h -= margin_y * 2;
}


static const char *uistrlayout_get_cell_ptr(uv_uistrlayout_st *this, int16_t index) {
	const char *ret = this->str;
	for (uint8_t i = 0; i < strlen(this->str); i++) {
		if (index == 0) {
			ret = &this->str[i];
			break;
		}
		if (this->str[i] == '|' || this->str[i] == '\n') {
			index--;
		}
	}
	return ret;
}

static int16_t uistrlayout_get_row_count_at(uv_uistrlayout_st *this, const char *cell_ptr) {
	int16_t ret = 0;
	while (cell_ptr != this->str) {
		if (*cell_ptr == '\n') {
			ret++;
		}
		cell_ptr--;
	}
	return ret;
}

// returns the number of cols on the row where *cell_ptr* is
static int16_t uistrlayout_get_col_count_at_cell(uv_uistrlayout_st *this, const char *cell_ptr) {
	int16_t ret = 1;
	while ((cell_ptr != this->str) &&
			(*cell_ptr != '\n')) {
		cell_ptr--;
	}
	if (*cell_ptr == '\n') {
		cell_ptr++;
	}
	// cell_ptr now points to the first cell on this row
	// calculate the number of cols until the end of the string or next row
	while ((cell_ptr < &this->str[strlen(this->str)]) &&
			(*cell_ptr != '\n')) {
		if (*cell_ptr == '|') {
			ret++;
		}
		else if (*cell_ptr == '#') {
			ret += MAX(strtol(cell_ptr + 1, NULL, 10) - 1, 0);
		}
		cell_ptr++;
	}
	return ret;
}

static int16_t uistrlayout_get_nth_cell_at_row(uv_uistrlayout_st *this, const char *cell_ptr) {
	int16_t ret = 0;
	cell_ptr--;
	while ((cell_ptr >= this->str) &&
			(*cell_ptr != '\n')) {
		if (*cell_ptr == '|') {
			ret++;
		}
		else if (*cell_ptr == '#') {
			ret += MAX(strtol(cell_ptr + 1, NULL, 10) - 1, 0);
		}
		else {

		}
		cell_ptr--;
	}
	return ret;
}


static uv_bounding_box_st uistrlayout_get_bb_from_cell(uv_uistrlayout_st *this, const char *cell) {
	uv_bounding_box_st bb = {};

	int16_t col_i = uistrlayout_get_nth_cell_at_row(this, cell);
	int16_t row_i = uistrlayout_get_row_count_at(this, cell);
	int16_t col_count = uistrlayout_get_col_count_at_cell(this, cell);
	int16_t span = 0;
	if (*cell == '#') {
		span = MAX(strtol(cell + 1, NULL, 10) - 1, 0);
	}
	if (this->horizontal) {
		bb.x = this->bb.x + col_i * this->bb.width / col_count + this->h_padding;
		bb.y = this->bb.y + row_i * this->bb.height / this->row_count + this->v_padding;
		bb.width = this->bb.width / col_count - this->h_padding * 2 +
				this->bb.width * span / col_count + this->h_padding * MAX(span - 1, 0);
		bb.height = this->bb.height / this->row_count - this->v_padding * 2;
	}
	else {
		bb.y = this->bb.y + col_i * this->bb.height / col_count + this->v_padding;
		bb.x = this->bb.x + row_i * this->bb.width / this->row_count + this->h_padding;
		bb.height = this->bb.height / col_count - this->v_padding * 2 +
				this->bb.height * span / col_count + this->v_padding * MAX(span - 1, 0);
		bb.width = this->bb.width / this->row_count - this->h_padding * 2;
	}


	return bb;
}


uv_bounding_box_st uv_uistrlayout_next(uv_uistrlayout_st *this) {
	uv_bounding_box_st bb;

	const char *cell = uistrlayout_get_cell_ptr(this, this->index);

	bb = uistrlayout_get_bb_from_cell(this, cell);

	this->index++;
	return bb;
}


static uv_bounding_box_st strlayout_find_next(uv_uistrlayout_st *this,
		const char *str, const char *c) {
	uv_bounding_box_st bb = {};
	char s[strlen(str) + 1];
	strcpy(s, str);
	uint16_t last_i = 0;
	const char *cell = NULL;
	// replace all cell changes and span markings with termination marks,
	// to make string finding easier
	uint16_t len = strlen(str);
	for (uint16_t i = 0; i < len + 1; i++) {
		if (str[i] == '|' || str[i] == '\n' || str[i] == '\0' || str[i] == '#') {
			if (str[i] == '#') {
				s[i] = '\0';
				// skip all numbers that follow span character '#'
				while(str[i + 1] >= '0' && str[i + 1] <= '9') {
					i++;
				}
			}
			s[i] = '\0';
			if (strcmp(&s[last_i], c) == 0) {
				cell = &str[last_i];
				// go back until the start of the cell.
				// Necessary to include '#' span characters
				while (cell != str &&
						*(cell - 1) != '\n' &&
						*(cell - 1) != '|') {
					cell--;
				}
				break;
			}
			else {
				last_i = i + 1;
			}
		}
	}
	// now *cell* should point to the start of the selected cell


	if (cell != NULL) {
		// calculate the found cell index
		const char *s = cell;
		int32_t index = 0;
		while (s >= this->str) {
			if (*s == '\n' || *s == '|') {
				index++;
			}
			s--;
		}
		this->index = index + 1;

		bb = uistrlayout_get_bb_from_cell(this, cell);
	}

	return bb;
}

uv_bounding_box_st uv_uistrlayout_find(uv_uistrlayout_st *this, const char *c) {
	return strlayout_find_next(this, this->str, c);
}


uv_bounding_box_st uv_uistrlayout_find_next(uv_uistrlayout_st *this, const char *c) {
	const char *str = uistrlayout_get_cell_ptr(this, this->index);
	return strlayout_find_next(this, str, c);
}

bool uv_uistrlayout_contains(uv_uistrlayout_st *this, const char *c) {
	bool ret = true;

	uv_bounding_box_st bb = uv_uistrlayout_find(this, c);
	if (bb.x == 0 &&
			bb.y == 0 &&
			bb.width == 0 &&
			bb.height == 0) {
		ret = false;
	}
	return ret;
}


#endif
