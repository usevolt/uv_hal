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


#include "uv_ui_common.h"
#include <string.h>

#if CONFIG_UI && CONFIG_UI_REMOTE


/// @brief: Single-buffer capture state machine.
///
/// The UI task appends encode_* output into *buf* while GATHERING, closes the
/// frame on frame_end and hands the buffer to the transport (TX). The transport
/// task drains the buffer read-only and calls frame_sent (-> WAIT). Gathering of
/// the next frame only resumes after the next frame boundary (dlswap), so a
/// buffer is never restarted in the middle of a screen.
typedef enum {
	// Not capturing; waiting for a clear (FRAME_BEGIN) to start a new frame.
	REMOTE_UI_IDLE = 0,
	// Appending the current frame's commands into buf.
	REMOTE_UI_GATHER,
	// buf holds a complete frame; the transport is draining it.
	REMOTE_UI_TX,
	// Transmit finished; wait for the next frame_end to realign, then IDLE.
	REMOTE_UI_WAIT
} remote_ui_state_e;


static struct {
	uint8_t buf[CONFIG_UI_REMOTE_BUFFER_SIZE];
	uint16_t buf_len;
	bool overflow;
	bool enabled;
	volatile remote_ui_state_e state;

	// hash of the last transmitted frame, for send-on-change suppression
	uint32_t last_hash;

	// bitmap asset registry (pointer -> sequential wire id)
	uv_uimedia_st *assets[CONFIG_UI_REMOTE_ASSET_MAX];
	uint16_t asset_count;

	// reverse input latch (written by transport task, read by UI task)
	volatile bool in_touched;
	volatile int16_t in_x;
	volatile int16_t in_y;
	volatile int16_t in_scroll;
	volatile char in_key;
} remote_ui;
#define this (&remote_ui)



static uint32_t fnv1a(const uint8_t *data, uint16_t len) {
	uint32_t h = 2166136261u;
	for (uint16_t i = 0; i < len; i++) {
		h ^= data[i];
		h *= 16777619u;
	}
	return h;
}


// --- append helpers (little-endian, bounds-checked) -------------------------

static void ap8(uint8_t v) {
	if (this->buf_len < sizeof(this->buf)) {
		this->buf[this->buf_len] = v;
		this->buf_len++;
	}
	else {
		this->overflow = true;
	}
}

static void ap16(uint16_t v) {
	ap8((uint8_t) (v & 0xFFu));
	ap8((uint8_t) ((v >> 8) & 0xFFu));
}

static void ap32(uint32_t v) {
	ap8((uint8_t) (v & 0xFFu));
	ap8((uint8_t) ((v >> 8) & 0xFFu));
	ap8((uint8_t) ((v >> 16) & 0xFFu));
	ap8((uint8_t) ((v >> 24) & 0xFFu));
}


static uint8_t resolve_font_id(const ui_font_st *font) {
	uint8_t id = UV_UI_REMOTE_FONT_UNKNOWN;
	for (uint8_t i = 0; i < UI_MAX_FONT_COUNT; i++) {
		if (font == &ui_fonts[i]) {
			id = i;
		}
		// ui_mono_fonts[] only exists on the host backends; the FT81X device has
		// a single font table, so its strings always resolve into ui_fonts[].
#if CONFIG_UI_OPENGL || CONFIG_UI_X11
		else if (font == &ui_mono_fonts[i]) {
			id = (uint8_t) (i | 0x80u);
		}
#endif
		else {
			// MISRA 15.7: no match on this index
		}
	}
	return id;
}


/// @brief: True when a new command may be appended to the current frame.
static bool gathering(void) {
	return this->enabled && (this->state == REMOTE_UI_GATHER);
}



void uv_ui_remote_init(void) {
	memset(this, 0, sizeof(*this));
	this->enabled = false;
	this->state = REMOTE_UI_IDLE;
}


void uv_ui_remote_reset(void) {
	this->buf_len = 0;
	this->overflow = false;
	this->state = REMOTE_UI_IDLE;
	this->last_hash = 0;
	this->asset_count = 0;
	this->in_touched = false;
	this->in_scroll = 0;
	this->in_key = '\0';
}


void uv_ui_remote_step(uint16_t step_ms) {
	// No periodic work needed for the source-side encoder; the transport pulls
	// frames on its own schedule. Placeholder for future asset streaming.
	(void) step_ms;
}


void uv_ui_remote_set_enabled(bool enabled) {
	if (enabled && !this->enabled) {
		this->enabled = true;
		this->state = REMOTE_UI_IDLE;
		this->last_hash = 0;
	}
	else if (!enabled && this->enabled) {
		this->enabled = false;
		this->state = REMOTE_UI_IDLE;
	}
	else {
		// no change
	}
}


bool uv_ui_remote_active(void) {
	return this->enabled;
}


uint16_t uv_ui_remote_register_bitmap(uv_uimedia_st *bitmap) {
	uint16_t id = UV_UI_REMOTE_ASSET_INVALID;
	if (bitmap != NULL) {
		for (uint16_t i = 0; i < this->asset_count; i++) {
			if (this->assets[i] == bitmap) {
				id = i;
			}
		}
		if ((id == UV_UI_REMOTE_ASSET_INVALID) &&
				(this->asset_count < CONFIG_UI_REMOTE_ASSET_MAX)) {
			this->assets[this->asset_count] = bitmap;
			id = this->asset_count;
			this->asset_count++;
		}
	}
	return id;
}


// --- transport pull interface ----------------------------------------------

bool uv_ui_remote_frame_pending(const uint8_t **data, uint16_t *len) {
	bool ret = false;
	if (this->state == REMOTE_UI_TX) {
		if (data != NULL) {
			*data = this->buf;
		}
		if (len != NULL) {
			*len = this->buf_len;
		}
		ret = true;
	}
	return ret;
}


void uv_ui_remote_frame_sent(void) {
	if (this->state == REMOTE_UI_TX) {
		this->state = REMOTE_UI_WAIT;
	}
}


// --- reverse input ----------------------------------------------------------

void uv_ui_remote_input_inject(uint8_t action, int16_t x, int16_t y,
		int16_t scroll, char key) {
	if (action == (uint8_t) UV_UI_REMOTE_INPUT_PRESS) {
		this->in_x = x;
		this->in_y = y;
		this->in_touched = true;
	}
	else {
		this->in_touched = false;
	}
	this->in_scroll = (int16_t) (this->in_scroll + scroll);
	if (key != '\0') {
		this->in_key = key;
	}
}


bool uv_ui_remote_get_touch(int16_t *x, int16_t *y) {
	bool ret = this->in_touched;
	if (ret) {
		if (x != NULL) {
			*x = this->in_x;
		}
		if (y != NULL) {
			*y = this->in_y;
		}
	}
	return ret;
}


int16_t uv_ui_remote_get_scroll(void) {
	int16_t ret = this->in_scroll;
	this->in_scroll = 0;
	return ret;
}


char uv_ui_remote_get_key(void) {
	char ret = this->in_key;
	this->in_key = '\0';
	return ret;
}


// --- encode hooks -----------------------------------------------------------

void uv_ui_remote_encode_clear(color_t c) {
	if (this->enabled &&
			((this->state == REMOTE_UI_IDLE) || (this->state == REMOTE_UI_GATHER))) {
		this->buf_len = 0;
		this->overflow = false;
		ap8((uint8_t) UV_UI_REMOTE_OP_FRAME_BEGIN);
		ap32(c);
		this->state = REMOTE_UI_GATHER;
	}
}


void uv_ui_remote_encode_frame_end(void) {
	if (this->enabled) {
		if (this->state == REMOTE_UI_GATHER) {
			ap8((uint8_t) UV_UI_REMOTE_OP_FRAME_END);
			if (this->overflow) {
				// frame did not fit; drop it and realign at the next clear
				this->state = REMOTE_UI_IDLE;
			}
			else {
				uint32_t hash = fnv1a(this->buf, this->buf_len);
				if (hash == this->last_hash) {
					// unchanged screen, nothing to send
					this->state = REMOTE_UI_IDLE;
				}
				else {
					this->last_hash = hash;
					this->state = REMOTE_UI_TX;
				}
			}
		}
		else if (this->state == REMOTE_UI_WAIT) {
			// previous frame finished transmitting; this dlswap is the clean
			// boundary at which we may resume capturing the next frame
			this->state = REMOTE_UI_IDLE;
		}
		else {
			// IDLE or TX: nothing to close
		}
	}
}


void uv_ui_remote_encode_point(int16_t x, int16_t y, color_t color, uint16_t diameter) {
	if (gathering()) {
		ap8((uint8_t) UV_UI_REMOTE_OP_POINT);
		ap16((uint16_t) x);
		ap16((uint16_t) y);
		ap16(diameter);
		ap32(color);
	}
}


void uv_ui_remote_encode_rrect(int16_t x, int16_t y, uint16_t w, uint16_t h,
		uint16_t radius, color_t color) {
	if (gathering()) {
		ap8((uint8_t) UV_UI_REMOTE_OP_RRECT);
		ap16((uint16_t) x);
		ap16((uint16_t) y);
		ap16(w);
		ap16(h);
		ap16(radius);
		ap32(color);
	}
}


void uv_ui_remote_encode_line(int16_t start_x, int16_t start_y,
		int16_t end_x, int16_t end_y, uint16_t width, color_t color) {
	if (gathering()) {
		ap8((uint8_t) UV_UI_REMOTE_OP_LINE);
		ap16((uint16_t) start_x);
		ap16((uint16_t) start_y);
		ap16((uint16_t) end_x);
		ap16((uint16_t) end_y);
		ap16(width);
		ap32(color);
	}
}


void uv_ui_remote_encode_linestrip(const uv_ui_linestrip_point_st *points,
		uint16_t point_count, uint16_t line_width, color_t color,
		uv_ui_strip_type_e type) {
	if (gathering() && (points != NULL)) {
		ap8((uint8_t) UV_UI_REMOTE_OP_LINESTRIP);
		ap8((uint8_t) type);
		ap16(line_width);
		ap32(color);
		ap16(point_count);
		for (uint16_t i = 0; i < point_count; i++) {
			ap16((uint16_t) points[i].x);
			ap16((uint16_t) points[i].y);
		}
	}
}


void uv_ui_remote_encode_polygon(const uv_ui_linestrip_point_st *points,
		uint16_t point_count, color_t color) {
	if (gathering() && (points != NULL)) {
		ap8((uint8_t) UV_UI_REMOTE_OP_POLYGON);
		ap32(color);
		ap16(point_count);
		for (uint16_t i = 0; i < point_count; i++) {
			ap16((uint16_t) points[i].x);
			ap16((uint16_t) points[i].y);
		}
	}
}


void uv_ui_remote_encode_string(char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color) {
	if (gathering()) {
		uint16_t slen = 0;
		if (str != NULL) {
			slen = (uint16_t) strlen(str);
		}
		ap8((uint8_t) UV_UI_REMOTE_OP_STRING);
		ap8(resolve_font_id(font));
		ap16((uint16_t) x);
		ap16((uint16_t) y);
		ap16((uint16_t) align);
		ap32(color);
		ap16(slen);
		for (uint16_t i = 0; i < slen; i++) {
			ap8((uint8_t) str[i]);
		}
	}
}


void uv_ui_remote_encode_mask(int16_t x, int16_t y, int16_t width, int16_t height) {
	if (gathering()) {
		ap8((uint8_t) UV_UI_REMOTE_OP_MASK);
		ap16((uint16_t) x);
		ap16((uint16_t) y);
		ap16((uint16_t) width);
		ap16((uint16_t) height);
	}
}


void uv_ui_remote_encode_bitmap(uv_uimedia_st *bitmap, int16_t x, int16_t y,
		int16_t w, int16_t h, uint32_t wrap, color_t color) {
	if (gathering()) {
		ap8((uint8_t) UV_UI_REMOTE_OP_BITMAP);
		ap16(uv_ui_remote_register_bitmap(bitmap));
		ap16((uint16_t) x);
		ap16((uint16_t) y);
		ap16((uint16_t) w);
		ap16((uint16_t) h);
		ap32(wrap);
		ap32(color);
	}
}


#endif /* CONFIG_UI && CONFIG_UI_REMOTE */
