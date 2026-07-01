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


#include <string.h>
#include "ui/uv_uiframewindow.h"
#include "uv_uiwindow.h"

#if CONFIG_UI

#define this ((uv_uiframewindow_st*)me)

// The frame line margin from the window edges.
#define FRAME_MARGIN		CONFIG_UI_FRAMEWINDOW_FRAME_MARGIN
// Padding between the frame line and the content area.
#define CONTENT_PAD			CONFIG_UI_FRAMEWINDOW_FRAME_MARGIN
// Distance from the left frame line to the start of the title text.
#define TITLE_LEFT_PAD		6
// Gap on each side of the title where the top line is not drawn.
#define TITLE_GAP			4


static void draw(void *me, const uv_bounding_box_st *pbb);


static bool has_title(uv_uiframewindow_st *me) {
	return (this->title != NULL) && (strlen(this->title) != 0);
}


// Horizontal inset (left/right) and bottom inset of the content area.
static int16_t side_inset(void) {
	return FRAME_MARGIN + CONTENT_PAD;
}

// Top inset of the content area. When a title is shown the content has to clear
// the title text that sits centered on the top frame line.
static int16_t top_inset(uv_uiframewindow_st *me) {
	int16_t ret = side_inset();
	if (has_title(this)) {
		int16_t th = uv_ui_get_string_height(this->title, this->font);
		int16_t with_title = FRAME_MARGIN + th / 2 + CONTENT_PAD;
		if (with_title > ret) {
			ret = with_title;
		}
	}
	return ret;
}


void uv_uiframewindow_init(void *me, uv_uiobject_st **obj_array,
		const uv_uistyle_st *style) {
	uv_uiwindow_init(this, obj_array, style);
	// transparent by default so the frame and children overlay the parent's
	// background instead of painting an opaque fill over it
	uv_uiwindow_set_transparent(this, true);
	this->title = NULL;
	this->font = style->font;
	this->text_c = style->text_color;
	this->frame_c = style->text_color;
	// offset the children so they sit inside the frame
	uv_uiwindow_set_content_bb_default_pos(this, side_inset(), top_inset(this));
	uv_uiobject_set_draw_callb(this, &draw);
}


void uv_uiframewindow_set_title(void *me, char *title) {
	this->title = title;
	// the title height affects the top content inset; recompute it
	uv_uiwindow_set_content_bb_default_pos(this, side_inset(), top_inset(this));
	uv_ui_refresh(this);
}


uv_bounding_box_st uv_uiframewindow_get_content_bb(void *me) {
	uv_bounding_box_st bb = *uv_uibb(me);
	int16_t side = side_inset();
	int16_t top = top_inset(this);
	bb.x += side;
	bb.y += top;
	bb.width -= 2 * side;
	bb.height -= top + side;
	return bb;
}


static void draw(void *me, const uv_bounding_box_st *pbb) {

	// super draw function (paints the background unless transparent)
	uv_uiwindow_draw(this, pbb);

	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;
	color_t c = this->frame_c;

	// corners of the frame rectangle
	int16_t lx = x + FRAME_MARGIN;
	int16_t rx = x + w - FRAME_MARGIN;
	int16_t ty = y + FRAME_MARGIN;
	int16_t by = y + h - FRAME_MARGIN;

	// left, right and bottom lines
	uv_ui_draw_line(lx, ty, lx, by, 1, c);
	uv_ui_draw_line(rx, ty, rx, by, 1, c);
	uv_ui_draw_line(lx, by, rx, by, 1, c);

	if (has_title(this)) {
		int16_t tx = lx + TITLE_LEFT_PAD;
		int16_t tw = uv_ui_get_string_width(this->title, this->font);
		// top line in two parts, leaving a gap for the title text
		if ((tx - TITLE_GAP) > lx) {
			uv_ui_draw_line(lx, ty, tx - TITLE_GAP, ty, 1, c);
		}
		int16_t rstart = tx + tw + TITLE_GAP;
		if (rstart < rx) {
			uv_ui_draw_line(rstart, ty, rx, ty, 1, c);
		}
		// title text, vertically centered on the top line
		uv_ui_draw_string(this->title, this->font, tx, ty, ALIGN_CENTER_LEFT,
				this->text_c);
	}
	else {
		// no title: a single continuous top line
		uv_ui_draw_line(lx, ty, rx, ty, 1, c);
	}

	_uv_uiwindow_draw_children(this, pbb);

	// scroll bars on top of the children, when the content overflows the frame
	uv_uiwindow_draw_scrollbars(this, pbb);
}


void uv_uiframewindow_clear(void *me) {
	void (*d)(void *, const uv_bounding_box_st *) =
			((uv_uiobject_st*) this)->vrtl_draw;
	uv_uiwindow_clear(this);
	uv_uiobject_set_draw_callb(this, d);
}


#endif
