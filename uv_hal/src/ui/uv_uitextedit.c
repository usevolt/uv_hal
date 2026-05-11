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


#include "ui/uv_uitextedit.h"
#include <string.h>

#if CONFIG_UI

#if !CONFIG_TARGET_LINUX
#include "ui/uv_uikeyboard.h"
#endif


static uv_uiobject_ret_e step(void *me, uint16_t step_ms);
static void touch(void *me, uv_touch_st *touch);


#define TITLE_OFFSET	4

#define this ((uv_uitextedit_st *) me)


void uv_uitextedit_init(void *me, char *buffer, uint16_t buf_len,
		uv_uitextedit_flags_e flags, const uv_uistyle_st *style) {
	uv_uilabel_init(this, style->font, ALIGN_CENTER, style->text_color, "");
	this->style = style;
	this->buffer = buffer;
	this->buf_len = buf_len;
	this->title = NULL;
	this->bg_color = style->bg_c;
	this->changed = false;
	this->flags = flags;
#if CONFIG_TARGET_LINUX
	this->editing = false;
	this->was_touched = false;
	this->blink_ms = 0;
#endif

	// ensure the buffer is null-terminated
	bool nullterm = false;
	for (uint16_t i = 0; i < buf_len; i++) {
		if (buffer[i] == '\0') {
			nullterm = true;
			break;
		}
	}
	if (!nullterm) {
		buffer[0] = '\0';
	}

	if ((flags & UITEXTEDIT_FLAG_ONELINE) != 0) {
		for (uint16_t i = 0; i < buf_len && buffer[i] != '\0'; i++) {
			if (buffer[i] == '\n' || buffer[i] == '\r') {
				buffer[i] = '\0';
				break;
			}
		}
	}

	// keep the inherited uilabel's str pointer pointing at our buffer so
	// uilabel-aware code paths see the live text.
	uv_uilabel_set_text(this, this->buffer);

	uv_uiobject_set_step_callb(this, &step);
	uv_uiobject_set_draw_callb(this, &uv_uitextedit_draw);
	uv_uiobject_set_touch_callb(this, &touch);
}


void uv_uitextedit_set_text(void *me, const char *text) {
	if (this->buf_len == 0 || text == NULL) {
		return;
	}
	strncpy(this->buffer, text, this->buf_len - 1);
	this->buffer[this->buf_len - 1] = '\0';
	if ((this->flags & UITEXTEDIT_FLAG_ONELINE) != 0) {
		for (uint16_t i = 0; this->buffer[i] != '\0'; i++) {
			if (this->buffer[i] == '\n' || this->buffer[i] == '\r') {
				this->buffer[i] = '\0';
				break;
			}
		}
	}
	uv_ui_refresh(this);
}


void uv_uitextedit_draw(void *me, const uv_bounding_box_st *pbb) {
	uint16_t x = uv_ui_get_xglobal(this),
			y = uv_ui_get_yglobal(this);
	uv_font_st *font = ((uv_uilabel_st*) this)->font;
	color_t text_color = ((uv_uilabel_st*) this)->color;

	uint16_t text_height = (this->buffer[0] == '\0') ?
			uv_ui_get_font_height(font) :
			uv_ui_get_string_height(this->buffer, font);
	uint16_t height = text_height + TITLE_OFFSET * 2;
	int16_t title_height = 0;
	if (this->title != NULL) {
		title_height = uv_ui_get_string_height(this->title, font);
	}
	y += (uv_uibb(this)->h - (height + title_height + TITLE_OFFSET)) / 2;

	uv_ui_draw_shadowrrect(x, y, uv_uibb(this)->width, height, 0,
			this->bg_color, uv_uic_brighten(this->bg_color, -30),
			uv_uic_brighten(this->bg_color, 30));

	uv_ui_draw_string(this->buffer, font,
			x + uv_uibb(this)->width / 2, y + height / 2 + CONFIG_UI_RADIUS,
			UI_ALIGN_CENTER, text_color);

#if CONFIG_TARGET_LINUX
	if (this->editing && this->blink_ms < UITEXTEDIT_CURSOR_BLINK_MS) {
		int16_t text_w = uv_ui_get_string_width(this->buffer, font);
		int16_t cursor_x = x + uv_uibb(this)->width / 2 + text_w / 2 + 1;
		uv_ui_draw_string("|", font, cursor_x,
				y + height / 2 + CONFIG_UI_RADIUS,
				UI_ALIGN_CENTER, text_color);
	}
#endif

	if (this->title) {
		uv_ui_draw_string(this->title, font,
				x + uv_uibb(this)->width / 2, y + height + TITLE_OFFSET,
				ALIGN_TOP_CENTER, text_color);
	}
}


static uv_uiobject_ret_e step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	this->changed = false;

#if CONFIG_TARGET_LINUX
	if (this->editing) {
		// blink the cursor: refresh on every phase boundary
		uint16_t prev_phase = this->blink_ms / UITEXTEDIT_CURSOR_BLINK_MS;
		this->blink_ms += step_ms;
		if (this->blink_ms >= UITEXTEDIT_CURSOR_BLINK_MS * 2) {
			this->blink_ms = 0;
		}
		uint16_t new_phase = this->blink_ms / UITEXTEDIT_CURSOR_BLINK_MS;
		if (prev_phase != new_phase) {
			uv_ui_refresh(this);
		}

		// drain typed characters
		char c;
		while ((c = uv_ui_get_key_press()) != '\0') {
			if (c == '\n' || c == '\r') {
				this->editing = false;
				this->changed = true;
				uv_ui_refresh(this);
				break;
			}
			else if (c == '\b' || c == 0x7f) {
				uint16_t len = strlen(this->buffer);
				if (len > 0) {
					this->buffer[len - 1] = '\0';
					uv_ui_refresh(this);
				}
			}
			else if (c == 0x1b) {
				// ESC: blur (single "blur commits" path)
				this->editing = false;
				this->changed = true;
				uv_ui_refresh(this);
				break;
			}
			else if (c >= 0x20 && c < 0x7f) {
				uint16_t len = strlen(this->buffer);
				if (len < this->buf_len - 1) {
					this->buffer[len] = c;
					this->buffer[len + 1] = '\0';
					uv_ui_refresh(this);
				}
			}
			else {
				// ignore other control characters
			}
		}

		// click-outside detection (rising edge on touch)
		int16_t tx = 0, ty = 0;
		bool touched = uv_ui_get_touch(&tx, &ty);
		if (touched && !this->was_touched) {
			int16_t gx = uv_ui_get_xglobal(this);
			int16_t gy = uv_ui_get_yglobal(this);
			int16_t w = uv_uibb(this)->width;
			int16_t h = uv_uibb(this)->height;
			if (tx < gx || tx > gx + w || ty < gy || ty > gy + h) {
				this->editing = false;
				this->changed = true;
				uv_ui_refresh(this);
			}
		}
		this->was_touched = touched;
	}
	else {
		this->was_touched = false;
		this->blink_ms = 0;
	}
#else
	(void) step_ms;
#endif

	return ret;
}


static void touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		touch->action = TOUCH_NONE;
#if CONFIG_TARGET_LINUX
		this->editing = true;
		this->blink_ms = 0;
		this->was_touched = true;
		// drop any keys queued before the field was focused
		while (uv_ui_get_key_press() != '\0') { }
		uv_ui_refresh(this);
#else
		// snapshot to detect whether the user actually changed anything
		char snapshot[this->buf_len];
		memcpy(snapshot, this->buffer, this->buf_len);
		uv_uikeyboard_show(this->title, this->buffer, this->buf_len, this->style);
		if ((this->flags & UITEXTEDIT_FLAG_ONELINE) != 0) {
			for (uint16_t i = 0; i < this->buf_len && this->buffer[i] != '\0'; i++) {
				if (this->buffer[i] == '\n' || this->buffer[i] == '\r') {
					this->buffer[i] = '\0';
					break;
				}
			}
		}
		if (strncmp(snapshot, this->buffer, this->buf_len) != 0) {
			this->changed = true;
		}
		uv_ui_refresh(this);
#endif
	}
}


#endif
