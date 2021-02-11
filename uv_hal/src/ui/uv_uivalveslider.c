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


#include <stdlib.h>
#include <ui/uv_uivalveslider.h>

#if CONFIG_UI

static void draw(void *me, const uv_bounding_box_st *pbb);
static void touch(void *me, uv_touch_st *touch);
static uv_uiobject_ret_e step(void *me, uint16_t step_ms);


#define POS_C C(0x2000FF00)
#define NEG_C C(0x20FF0000)
#define OUTBOUNDS_C	C(0x10FFFFFF)
#define POSTEXT_C C(0xFF00FF00)
#define NEGTEXT_C C(0xFFFF0000)



#define this ((uv_uivalveslider_st*) me)


static void get_handle_x_coords(void *me, int16_t x, int16_t y,
		int16_t w, int16_t h, int16_t *x_arr) {
	int16_t handle_w = CONFIG_UIVALVESLIDER_HANDLE_WIDTH;
	int16_t handle_x[UIVALVESLIDER_HANDLE_COUNT] = {
			uv_lerpi(
						uv_reli(this->handle_values[UIVALVESLIDER_HANDLE_POS_MIN], 0, this->max_val),
						x + w / 2 + (handle_w / 2), x + w - (handle_w * 3 / 2) - this->horiz_padding),
			uv_lerpi(
						uv_reli(this->handle_values[UIVALVESLIDER_HANDLE_POS_MAX], 0, this->max_val),
						x + w / 2 + (handle_w * 3 / 2), x + w - (handle_w / 2) - this->horiz_padding),
			uv_lerpi(
						uv_reli(this->handle_values[UIVALVESLIDER_HANDLE_NEG_MIN], 0, this->min_val),
						x + w / 2 - (handle_w / 2), x + (handle_w * 3 / 2) + this->horiz_padding),
			uv_lerpi(
						uv_reli(this->handle_values[UIVALVESLIDER_HANDLE_NEG_MAX], 0, this->min_val),
						x + w / 2 - (handle_w * 3 / 2), x + (handle_w / 2) + this->horiz_padding)
	};
	memcpy(x_arr, handle_x, sizeof(handle_x));

	return;
}


static void draw_handle(void *me, int16_t handle_index, int16_t x_global, int16_t x_mid,
		int16_t y_global, int16_t height, bool selected, bool disabled) {
	color_t handle_c;
	color_t text_c;
	if (selected) {
		handle_c = this->selected_handle_c;
		text_c = this->text_c;
	}
	else if (!disabled &&
			this->selected_handle == UIVALVESLIDER_HANDLE_COUNT) {
		handle_c = this->handle_c;
		text_c = this->text_c;
	}
	else {
		handle_c = uv_uic_grayscale(this->handle_c);
		text_c = uv_uic_brighten(this->text_c, -0x80);
	}
	uv_ft81x_draw_shadowrrect(x_mid - CONFIG_UIVALVESLIDER_HANDLE_WIDTH / 2,
			y_global, CONFIG_UIVALVESLIDER_HANDLE_WIDTH, height,
			CONFIG_UI_RADIUS, handle_c,
			uv_uic_brighten(handle_c, 30),
			uv_uic_brighten(handle_c, -30));

	uv_ft81x_draw_string(this->handle_strs[handle_index], this->font,
			x_mid, y_global + height / 2 -
			uv_ft81x_get_string_height(this->handle_strs[handle_index], this->font) - 2,
			ALIGN_TOP_CENTER, text_c);
	char str[32];
	snprintf(str, sizeof(str), "%i", this->handle_values[handle_index]);
	uv_ft81x_draw_string(str, this->font,
			x_mid, y_global + height / 2 + 2,
			ALIGN_TOP_CENTER, text_c);
}


static void draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x, y, w, h;
	x = uv_ui_get_xglobal(this);
	y = uv_ui_get_yglobal(this);
	w = uv_uibb(this)->width;
	h = uv_uibb(this)->height;
	int16_t globh = h;
	if (this->cursor_text != NULL) {
		h -= uv_ft81x_get_string_height(this->cursor_text, this->font);
	}
	if (this->title != NULL) {
		h -= uv_ft81x_get_string_height(this->title, this->font);
	}
	int16_t handlestr_height = 0;
	for (uint8_t i = 0; i < UIVALVESLIDER_HANDLE_COUNT; i++) {
		int16_t height = uv_ft81x_get_string_height(this->handle_strs[i], this->font);
		if (height > handlestr_height) {
			handlestr_height = height;
		}
	}

	int16_t handle_x[UIVALVESLIDER_HANDLE_COUNT];
	get_handle_x_coords(me, x, y, w, h, handle_x);

	// draw the triangles
	uv_ft81x_linestrip_point_st p[2];
	p[0].x = x + w / 2;
	p[0].y = y + h - 2;
	// point 1 is set twice as far, in order to create vertical grey areas when
	// horiz_padding is in use
	p[1].x = x + (w - this->horiz_padding) + (w / 2 - this->horiz_padding);
	p[1].y = y - h - 2;
	uint8_t linestrip_len = sizeof(p) / sizeof(p[0]);
	uint8_t h_off = 2;

	uv_ft81x_set_mask(x + w / 2, y, handle_x[UIVALVESLIDER_HANDLE_POS_MIN] - (x + w / 2),
			h - h_off);
	uv_ft81x_draw_linestrip(p, linestrip_len, 1,
			this->outbounds_c, FT81X_STRIP_TYPE_BELOW);

	uv_ft81x_set_mask(handle_x[UIVALVESLIDER_HANDLE_POS_MIN], y,
			handle_x[UIVALVESLIDER_HANDLE_POS_MAX] - handle_x[UIVALVESLIDER_HANDLE_POS_MIN],
			h - h_off);
	uv_ft81x_draw_linestrip(p, linestrip_len, 1,
			(this->selected_handle == UIVALVESLIDER_HANDLE_POS_MAX ||
					this->selected_handle == UIVALVESLIDER_HANDLE_POS_MIN) ?
							uv_uic_alpha(this->positive_c, 0x20) :
							(this->selected_handle == UIVALVESLIDER_HANDLE_COUNT) ?
									this->positive_c : uv_uic_grayscale(this->positive_c),
									FT81X_STRIP_TYPE_BELOW);

	uv_ft81x_set_mask(handle_x[UIVALVESLIDER_HANDLE_POS_MAX], y + 2,
			w - (handle_x[UIVALVESLIDER_HANDLE_POS_MAX] - x),
			h - h_off * 2);
	uv_ft81x_draw_linestrip(p, linestrip_len, 1,
			this->outbounds_c, FT81X_STRIP_TYPE_BELOW);

	p[1].x = x + this->horiz_padding - (w / 2 - this->horiz_padding);
	p[1].y = y - h;

	uv_ft81x_set_mask(handle_x[UIVALVESLIDER_HANDLE_NEG_MIN], y, w, h - h_off);
	uv_ft81x_draw_linestrip(p, linestrip_len, 1,
			this->outbounds_c, FT81X_STRIP_TYPE_BELOW);

	uv_ft81x_set_mask(handle_x[UIVALVESLIDER_HANDLE_NEG_MAX], y,
			handle_x[UIVALVESLIDER_HANDLE_NEG_MIN] - handle_x[UIVALVESLIDER_HANDLE_NEG_MAX],
			h - h_off);
	uv_ft81x_draw_linestrip(p, linestrip_len, 1,
			(this->selected_handle == UIVALVESLIDER_HANDLE_NEG_MAX ||
					this->selected_handle == UIVALVESLIDER_HANDLE_NEG_MIN) ?
							uv_uic_alpha(this->negative_c, 0x20) :
							((this->selected_handle == UIVALVESLIDER_HANDLE_COUNT &&
									!this->unidir) ?
									this->negative_c : uv_uic_grayscale(this->negative_c)),
									FT81X_STRIP_TYPE_BELOW);

	uv_ft81x_set_mask(x, y,
			handle_x[UIVALVESLIDER_HANDLE_NEG_MAX] - x, h - h_off * 2);
	uv_ft81x_draw_linestrip(p, linestrip_len, 1,
			this->outbounds_c, FT81X_STRIP_TYPE_BELOW);

	uv_ft81x_set_mask(x, y, w, globh);

	// draw the arrows
	int16_t handle_w = CONFIG_UIVALVESLIDER_HANDLE_WIDTH;
	if (this->selected_handle != UIVALVESLIDER_HANDLE_COUNT &&
			this->leftarrow_media != NULL &&
			this->rightarrow_media != NULL) {
		int16_t bitmap_w = uv_uimedia_get_bitmapwidth(this->leftarrow_media);
		int16_t bitmap_h = uv_uimedia_get_bitmapheight(this->leftarrow_media);
		uv_ft81x_draw_bitmap(
				this->leftarrow_media, handle_x[this->selected_handle] - handle_w / 2 - bitmap_w,
				y + h / 2 - bitmap_h / 2);
		uv_ft81x_draw_bitmap(
				this->rightarrow_media, handle_x[this->selected_handle] + handle_w / 2,
				y + h / 2 - bitmap_h / 2);
	}

	// draw the handles
	for (uint8_t i = 0; i < UIVALVESLIDER_HANDLE_COUNT; i++) {
		draw_handle(this, i, x, handle_x[i], y, h, this->selected_handle == i,
				this->unidir && (i == UIVALVESLIDER_HANDLE_NEG_MAX ||
						i == UIVALVESLIDER_HANDLE_NEG_MIN));
	}

	// draw the cursor if assigned
	if (this->cursor_text != NULL &&
			this->cursor_position >= -100 &&
			this->cursor_position <= 100) {
		int16_t cursor_x;
		color_t cursor_c = this->text_c;
		if (this->cursor_position > 0) {
			cursor_x = uv_lerpi(
					uv_reli(this->cursor_position, 0, this->max_val),
					handle_x[UIVALVESLIDER_HANDLE_POS_MIN] + handle_w / 2,
					handle_x[UIVALVESLIDER_HANDLE_POS_MAX] - handle_w / 2);
			cursor_c = POSTEXT_C;
		}
		else if (this->cursor_position < 0) {
			cursor_x = uv_lerpi(
					uv_reli(this->cursor_position, 0, this->min_val),
					handle_x[UIVALVESLIDER_HANDLE_NEG_MIN] - handle_w / 2,
							 handle_x[UIVALVESLIDER_HANDLE_NEG_MAX] + handle_w / 2);
			cursor_c = NEGTEXT_C;
		}
		else {
			cursor_x = x + w / 2;
		}
		uv_ft81x_draw_line(cursor_x, y, cursor_x, y + h, 1, cursor_c);
		uv_ft81x_draw_string(this->cursor_text, this->font,
				cursor_x, y + h, ALIGN_TOP_CENTER, cursor_c);
	}

	// lastly draw the title if one is assigned
	if (this->title != NULL) {
		uv_ft81x_draw_string(this->title, this->font, x + w / 2,
				y + globh - uv_ft81x_get_string_height(this->title, this->font),
				ALIGN_TOP_CENTER, this->text_c);
	}

}



void uv_uivalveslider_init(void *me, int16_t min_val, int16_t max_val,
		uv_uimedia_st *leftarrow_media, uv_uimedia_st *rightarrow_media,
		char *handle_strs[], int16_t handle_values[],
		const uv_uistyle_st *style) {
	uv_uiobject_init(this);
	uv_uiobject_set_draw_callb(this, &draw);
	uv_uiobject_set_step_callb(this, &step);
	uv_uiobject_set_touch_callb(this, &touch);

	this->min_val = min_val;
	this->max_val = max_val;
	this->title = NULL;
	this->unidir = false;
	this->cursor_text = NULL;
	this->cursor_position = INT16_MAX;
	this->horiz_padding = 0;
	this->leftarrow_media = leftarrow_media;
	this->rightarrow_media = rightarrow_media;
	memcpy(this->handle_strs, handle_strs, sizeof(this->handle_strs));
	memcpy(this->handle_values, handle_values, sizeof(this->handle_values));
	this->positive_c = POS_C;
	this->negative_c = NEG_C;
	this->outbounds_c = OUTBOUNDS_C;
	this->handle_c = style->bg_c;
	this->selected_handle_c = style->fg_c;
	this->font = style->font;
	this->text_c = style->text_color;
	this->value_changed = false;
	this->selected_handle = UIVALVESLIDER_HANDLE_COUNT;
	this->inc_step = MAX((this->max_val - this->min_val) / 200, 1);
}



static void set_selected_handle(void *me, int16_t value) {
	this->handle_values[this->selected_handle] = value;

	if (this->selected_handle == UIVALVESLIDER_HANDLE_NEG_MAX ||
			this->selected_handle == UIVALVESLIDER_HANDLE_NEG_MIN) {
		LIMITS(this->handle_values[this->selected_handle], this->min_val, 0);
	}
	else {
		LIMITS(this->handle_values[this->selected_handle], 0, this->max_val);
	}

	if (this->selected_handle == UIVALVESLIDER_HANDLE_POS_MIN) {
		if (this->handle_values[this->selected_handle] >
				this->handle_values[this->selected_handle + 1]) {
			this->handle_values[this->selected_handle + 1] =
					this->handle_values[this->selected_handle];
		}
	}
	else if (this->selected_handle == UIVALVESLIDER_HANDLE_NEG_MIN) {
		if (this->handle_values[this->selected_handle] <
				this->handle_values[this->selected_handle + 1]) {
			this->handle_values[this->selected_handle + 1] =
					this->handle_values[this->selected_handle];
		}
	}
	else if (this->selected_handle == UIVALVESLIDER_HANDLE_POS_MAX) {
		if (this->handle_values[this->selected_handle] <
				this->handle_values[this->selected_handle - 1]) {
			this->handle_values[this->selected_handle - 1] =
					this->handle_values[this->selected_handle];
		}
	}
	else {
		// UIVALVESLIDER_HANDLE_NEG_MAX
		if (this->handle_values[this->selected_handle] >
				this->handle_values[this->selected_handle - 1]) {
			this->handle_values[this->selected_handle - 1] =
					this->handle_values[this->selected_handle];
		}
	}


	if (this->unidir) {
		this->handle_values[UIVALVESLIDER_HANDLE_NEG_MIN] =
				-this->handle_values[UIVALVESLIDER_HANDLE_POS_MIN];
		this->handle_values[UIVALVESLIDER_HANDLE_NEG_MAX] =
				-this->handle_values[UIVALVESLIDER_HANDLE_POS_MAX];
	}

	uv_ui_refresh(this);
	this->value_changed = true;
}



static void touch(void *me, uv_touch_st *touch) {
	int16_t handle_x[UIVALVESLIDER_HANDLE_COUNT];
	int16_t xglob = uv_ui_get_xglobal(me);
	get_handle_x_coords(me, xglob, uv_ui_get_yglobal(me),
			uv_uibb(me)->width, uv_uibb(me)->height, handle_x);
	if (this->selected_handle == UIVALVESLIDER_HANDLE_COUNT) {
		// handle is not selected, click selects the handle
		if (touch->action == TOUCH_PRESSED) {
			for (uint8_t i = 0;
					(this->unidir) ?
					(i <= UIVALVESLIDER_HANDLE_POS_MAX) : (i < UIVALVESLIDER_HANDLE_COUNT);
					i++) {
				if (touch->x > handle_x[i] - CONFIG_UIVALVESLIDER_HANDLE_WIDTH / 2  - xglob &&
						touch->x < handle_x[i] + CONFIG_UIVALVESLIDER_HANDLE_WIDTH / 2 - xglob) {
					// this handle was touched
					this->selected_handle = i;
					touch->action = TOUCH_NONE;
					this->drag_start_val = this->handle_values[this->selected_handle];
					this->drag_x = 0;
					uv_delay_end(&this->inc_delay);
					uv_ui_refresh(this);
					break;
				}
			}
		}

	}
	else {
		// handle has been selected
		if (touch->action == TOUCH_PRESSED) {
			uv_delay_init(&this->inc_delay, UISLIDER_LONGPRESS_DELAY_MS);
			if (touch->x < handle_x[this->selected_handle] -
					CONFIG_UIVALVESLIDER_HANDLE_WIDTH / 2 - xglob) {
				// decrease the value
				set_selected_handle(this,
						this->handle_values[this->selected_handle] - this->inc_step);
				this->inc = false;
			}
			else if (touch->x > handle_x[this->selected_handle] +
					CONFIG_UIVALVESLIDER_HANDLE_WIDTH / 2 - xglob) {
				// increase the value
				set_selected_handle(this,
						this->handle_values[this->selected_handle] + this->inc_step);
				this->inc = true;
			}
			else {
				// the selected handle was clicked. De-select the current handle
				this->selected_handle = UIVALVESLIDER_HANDLE_COUNT;
				uv_ui_refresh(this);
			}
		}
		else if (touch->action == TOUCH_DRAG) {
			this->drag_x += touch->x;
			uv_delay_end(&this->inc_delay);
			int16_t w = uv_uibb(this)->width / 2 -
					this->horiz_padding -
					CONFIG_UIVALVESLIDER_HANDLE_WIDTH * 2;
			int16_t rel = uv_reli(this->drag_x, 0, w);
			int16_t val;
			if (this->selected_handle == UIVALVESLIDER_HANDLE_POS_MAX ||
					this->selected_handle == UIVALVESLIDER_HANDLE_POS_MIN) {
				val = this->drag_start_val + uv_lerpi(rel, 0, this->max_val);
			}
			else {
				val = this->drag_start_val + uv_lerpi(-rel, 0, this->min_val);
			}
			set_selected_handle(this, val);
		}
		else if (touch->action != TOUCH_IS_DOWN) {
			uv_delay_end(&this->inc_delay);
		}
		else {
			// update pressed coordinate when is_down action happens
			this->drag_start_val = this->handle_values[this->selected_handle];
			this->drag_x = 0;
		}
		touch->action = TOUCH_NONE;
	}
}



static uv_uiobject_ret_e step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	this->value_changed = false;

	if (this->selected_handle != UIVALVESLIDER_HANDLE_COUNT) {
		if (uv_delay(&this->inc_delay, step_ms)) {
			uv_delay_init(&this->inc_delay, UISLIDER_LONGPRESS_MIN_DELAY_MS);
			set_selected_handle(this,
					this->handle_values[this->selected_handle] +
					this->inc_step * (this->inc ? 1 : -1));
			uv_ui_refresh(this);
		}
	}

	return ret;
}





int16_t uv_uivalveslider_get_selected_handle_value(void *me) {
	int16_t ret = 0;
	if (this->selected_handle != UIVALVESLIDER_HANDLE_COUNT) {
		ret = this->handle_values[this->selected_handle];
	}
	return ret;
}




#endif
