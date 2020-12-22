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


#define POS_C C(0x2200FF00)
#define POSTEXT_C C(0xFF00FF00)
#define NEG_C C(0x22FF0000)
#define NEGTEXT_C C(0xFFFF0000)



#define this ((uv_uivalveslider_st*) me)



static void draw_handle(void *me, int16_t handle_index, int16_t x_global, int16_t x_mid,
		int16_t y_global, int16_t handlestr_height, int16_t height, bool selected) {
	color_t handle_c = (selected) ?
			this->handle_c : uv_uic_grayscale(this->handle_c);
	uv_ft81x_draw_shadowrrect(x_global + x_mid - CONFIG_UIVALVESLIDER_HANDLE_WIDTH / 2,
			y_global + handlestr_height, CONFIG_UIVALVESLIDER_HANDLE_WIDTH,
			height - handlestr_height - uv_ft81x_get_font_height(this->font),
			CONFIG_UI_RADIUS, handle_c,
			uv_uic_brighten(handle_c, 30),
			uv_uic_brighten(handle_c, -30));

	color_t text_c = (selected) ?
			this->text_c :
			uv_uic_brighten(uv_uic_grayscale(this->text_c), -30);
	uv_ft81x_draw_string(this->handle_strs[handle_index], this->font,
			x_global + x_mid, y_global, ALIGN_TOP_CENTER, text_c);
	char str[32];
	snprintf(str, sizeof(str), "%i", this->handle_values[handle_index]);
	uv_ft81x_draw_string(str, this->font,
			x_global + x_mid, y_global + height - uv_ft81x_get_font_height(this->font),
			ALIGN_TOP_CENTER, text_c);
}


static void draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x, y, w, h;
	x = uv_ui_get_xglobal(this);
	y = uv_ui_get_yglobal(this);
	w = uv_uibb(this)->width;
	h = uv_uibb(this)->height;
	int16_t handlestr_height = 0;
	for (uint8_t i = 0; i < UIVALVESLIDER_HANDLE_COUNT; i++) {
		int16_t height = uv_ft81x_get_string_height(this->handle_strs[i], this->font);
		if (height > handlestr_height) {
			handlestr_height = height;
		}
	}
	int16_t val_height = uv_ft81x_get_font_height(this->font);
	int16_t handle_x[UIVALVESLIDER_HANDLE_COUNT] = {
			uv_lerpi(
						uv_reli(this->handle_values[UIVALVESLIDER_HANDLE_POS_MIN], 0, this->max_val),
						x + w / 2, x + w),
			uv_lerpi(
						uv_reli(this->handle_values[UIVALVESLIDER_HANDLE_POS_MAX], 0, this->max_val),
						x + w / 2, x + w),
			uv_lerpi(
						uv_reli(this->handle_values[UIVALVESLIDER_HANDLE_NEG_MIN], 0, this->min_val),
						x + w / 2, x),
			uv_lerpi(
						uv_reli(this->handle_values[UIVALVESLIDER_HANDLE_NEG_MAX], 0, this->min_val),
						x + w / 2, x)
	};

	// draw the triangles

	// draw the handles
	for (uint8_t i = 0; i < UIVALVESLIDER_HANDLE_COUNT; i++) {
		if (this->selected_handle != i) {
			draw_handle(this, i, x, handle_x[i], y, handlestr_height, h, false);
		}
	}
	// lastly draw the selected handle to be on top of everything
	if (this->selected_handle != UIVALVESLIDER_HANDLE_COUNT) {
		draw_handle(this, this->selected_handle, x, handle_x[this->selected_handle],
				y, handlestr_height, h, true);
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
	this->leftarrow_media = leftarrow_media;
	this->rightarrow_media = rightarrow_media;
	memcpy(this->handle_strs, handle_strs, sizeof(this->handle_strs));
	memcpy(this->handle_values, handle_values, sizeof(this->handle_values));
	this->positive_c = POS_C;
	this->negative_c = NEG_C;
	this->handle_c = style->fg_c;
	this->font = style->font;
	this->text_c = style->text_color;
	this->value_changed = false;
	this->selected_handle = UIVALVESLIDER_HANDLE_COUNT;
	this->inc_step = MAX((this->max_val - this->min_val) / 20, 1);
}



static void add_to_selected_handle(void *me, int16_t value) {
	this->handle_values[this->selected_handle] += value;
	if (this->selected_handle == UIVALVESLIDER_HANDLE_NEG_MAX ||
			this->selected_handle == UIVALVESLIDER_HANDLE_NEG_MIN) {
		LIMITS(this->handle_values[this->selected_handle], this->min_val, 0);
	}
	else {
		LIMITS(this->handle_values[this->selected_handle], 0, this->max_val);
	}

	if (this->selected_handle == UIVALVESLIDER_HANDLE_NEG_MIN ||
			this->selected_handle == UIVALVESLIDER_HANDLE_POS_MIN) {
		if (this->handle_values[this->selected_handle] >
				this->handle_values[this->selected_handle + 1]) {
			this->handle_values[this->selected_handle + 1] =
					this->handle_values[this->selected_handle];
		}
	}
	else {
		if (this->handle_values[this->selected_handle] <
				this->handle_values[this->selected_handle - 1]) {
			this->handle_values[this->selected_handle - 1] =
					this->handle_values[this->selected_handle];
		}
	}
	uv_ui_refresh(this);
	this->value_changed = true;
}



static void touch(void *me, uv_touch_st *touch) {
	if (this->selected_handle == UIVALVESLIDER_HANDLE_COUNT) {
		// handle is not selected, click selects the handle
		if (touch->action == TOUCH_CLICKED) {
			for (uint8_t i = 0; i < UIVALVESLIDER_HANDLE_COUNT; i++) {
				int16_t handle_x = uv_lerpi(
						uv_reli(this->handle_values[i], this->min_val, this->max_val),
						0, uv_uibb(this)->width);
				if (touch->x > handle_x - CONFIG_UIVALVESLIDER_HANDLE_WIDTH / 2 &&
						touch->x < handle_x + CONFIG_UIVALVESLIDER_HANDLE_WIDTH / 2) {
					// this handle was touched
					this->selected_handle = i;
					touch->action = TOUCH_NONE;
					uv_ui_refresh(this);
					break;
				}
			}
		}

	}
	else {
		// handle has been selected
		int16_t handle_x = uv_lerpi(
				uv_reli(this->handle_values[this->selected_handle], this->min_val, this->max_val),
				0, uv_uibb(this)->width);

		if (touch->action == TOUCH_CLICKED ||
				touch->action == TOUCH_PRESSED) {
			if (touch->x < handle_x - CONFIG_UIVALVESLIDER_HANDLE_WIDTH / 2) {
				// decrease the value
				add_to_selected_handle(this, -this->inc_step);
			}
			else if (touch->x > handle_x + CONFIG_UIVALVESLIDER_HANDLE_WIDTH / 2) {
				// increase the value
				add_to_selected_handle(this, this->inc_step);
			}
			else {
				if (touch->action == TOUCH_CLICKED) {
					// the selected handle was clicked. De-select the current handle
					// and select the handle under this one if possible.
					uint8_t i = this->selected_handle + 1;
					this->selected_handle = 0;
					for (; i < UIVALVESLIDER_HANDLE_COUNT; i++) {
						int16_t handle_x = uv_lerpi(
								uv_reli(this->handle_values[i], this->min_val, this->max_val),
								0, uv_uibb(this)->width);
						if (touch->x > handle_x - CONFIG_UIVALVESLIDER_HANDLE_WIDTH / 2 &&
								touch->x < handle_x + CONFIG_UIVALVESLIDER_HANDLE_WIDTH / 2) {
							this->selected_handle = i;
							uv_ui_refresh(this);
							break;
						}
					}
				}
			}
		}
		touch->action = TOUCH_NONE;
	}
}



static uv_uiobject_ret_e step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	this->value_changed = false;

	return ret;
}






#endif
