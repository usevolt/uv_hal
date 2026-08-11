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

	// The display was drawn while this buffer was busy with an earlier frame,
	// so what the sink is looking at is out of date. Only ever set by a screen
	// that was actually drawn and actually missed: a display that is not
	// changing sets it never, and cannot ask for a redraw it does not need.
	bool missed;

	// A bitmap the sink asked for, caught as it was drawn. That is the only
	// place its file name is known - the id on the wire is a hash and cannot be
	// turned back into a file - so it is kept here until the frame ends and the
	// transfer can open.
	const char *pending_name;
	uint32_t pending_id;

	// --- assets the sink has asked for --------------------------------------
	// Written by the transport task, read and cleared by the UI task.
	struct {
		volatile uint8_t kind;
		volatile uint32_t id;
		volatile bool pending;
	} wanted[CONFIG_UI_REMOTE_ASSET_REQ_MAX];

	uv_ui_remote_asset_size_t asset_size_callb;
	uv_ui_remote_asset_read_t asset_read_callb;

	// The chunk being handed to the transport. Filled on the UI task at a frame
	// boundary (the provider reads flash, which the UI task owns) and drained by
	// the transport task, so only one chunk is in flight at a time.
	struct {
		uint8_t buf[UV_UI_REMOTE_ASSET_HDR_LEN + UV_UI_REMOTE_ASSET_CHUNK];
		uint16_t len;
		uint8_t flags;
		volatile bool ready;
		// the transfer in progress
		bool active;
		uint8_t kind;
		uint32_t id;
		// what the provider is asked for; NULL for a font, which is built here
		const char *name;
		uint32_t total;
		uint32_t offset;
	} tx_asset;

	// a font asset is built here when its transfer opens, and chunked out of it
	uint8_t font_body[UV_UI_REMOTE_FONT_BODY_HDR_LEN];

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


/// @brief: True when a new command may be appended to the current frame,
/// opening one first if a draw arrives with no frame in progress.
///
/// A normal screen opens its frame by clearing (uv_ui_clear -> FRAME_BEGIN).
/// The dialogs that run their own exec loop - the save dialog, the accept and
/// password dialogs, the numpad, the keyboard - never clear: they draw their
/// window over what is already on the display list and swap. Without the
/// implicit open below, the encoder sat in IDLE through all of it, every one of
/// their draw calls was dropped, and the remote view froze on the last full
/// screen until the dialog was dismissed.
///
/// Such a frame is opened with FRAME_BEGIN_KEEP, which tells the sink to draw
/// over the frame it already has rather than starting from a cleared screen -
/// the same thing the device is doing to its own display list.
static bool gathering(void) {
	if (this->enabled && (this->state == REMOTE_UI_IDLE)) {
		this->buf_len = 0;
		this->overflow = false;
		ap8((uint8_t) UV_UI_REMOTE_OP_FRAME_BEGIN_KEEP);
		this->state = REMOTE_UI_GATHER;
	}
	else {
		// a frame is already open, or we are not capturing at all
	}
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
	this->pending_name = NULL;
	for (uint8_t i = 0; i < CONFIG_UI_REMOTE_ASSET_REQ_MAX; i++) {
		this->wanted[i].pending = false;
	}
	this->tx_asset.ready = false;
	this->tx_asset.active = false;
	this->missed = false;
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
		// a sink that has just connected has nothing at all, so it is owed a
		// screen by definition
		this->missed = true;
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


void uv_ui_remote_request_frame(void) {
	this->missed = true;
	// Whoever asks for a frame is looking at nothing, so what was sent last is
	// no measure of what they have. Without forgetting it, a redraw of an
	// unchanged screen hashes the same as the one already sent and is dropped
	// as a duplicate - and the sink that just asked keeps its empty window
	// until something on the display happens to move.
	this->last_hash = 0;
}


bool uv_ui_remote_redraw_wanted(void) {
	// Not while transmitting: a redraw then could not be captured either, and
	// would only be work thrown away. Once the buffer frees up the answer turns
	// true and stays true until a screen has actually been captured.
	return this->enabled && this->missed &&
			((this->state == REMOTE_UI_IDLE) || (this->state == REMOTE_UI_WAIT));
}


uint32_t uv_ui_remote_bitmap_id(const uv_uimedia_st *bitmap) {
	uint32_t id = UV_UI_REMOTE_ASSET_INVALID_ID;
	if ((bitmap != NULL) && (bitmap->filename != NULL) &&
			(bitmap->filename[0] != '\0')) {
		id = fnv1a((const uint8_t *) bitmap->filename,
				(uint16_t) strlen(bitmap->filename));
	}
	else {
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


// --- serving assets ---------------------------------------------------------

/// @brief: Builds the body of a font asset into this->font_body.
///
/// Fonts are served from here rather than through the installed provider
/// because a font is not stored anywhere the backend has to look it up: it is
/// ui_fonts[], which this module can already see. The provider exists for
/// assets that do live in storage - bitmap files - and knows nothing about
/// fonts.
///
/// Metrics only, for now. The FT81X ROM fonts keep their glyphs inside the chip
/// with no copy anywhere else, so those could never be sent whole; the custom
/// fonts could be (their L4 atlas is a file in external flash) and the GLYPHS
/// flag is reserved for that, but until then a sink lays text out with the
/// device's real height and advances and draws it with a face of its own.
///
/// @return: body length, or 0 when there is no such font.
static uint16_t font_body_build(uint32_t id) {
	uint16_t ret = 0;
	bool mono = ((id & 0x80u) != 0u);
	uint8_t index = (uint8_t) (id & 0x7Fu);
	const ui_font_st *font = NULL;

	if (index < UI_MAX_FONT_COUNT) {
#if CONFIG_UI_OPENGL || CONFIG_UI_X11
		font = mono ? &ui_mono_fonts[index] : &ui_fonts[index];
#else
		// one font table on the device; a mono request resolves to the same
		(void) mono;
		font = &ui_fonts[index];
#endif
	}
	else {
	}

	if ((font != NULL) && (font->char_height > 0)) {
		uint8_t *p = this->font_body;
		p[0] = (uint8_t) (font->char_height & 0xFFu);
		p[1] = (uint8_t) ((font->char_height >> 8) & 0xFFu);
		p[2] = 0;			// flags: no glyph atlas
		p[3] = 0; p[4] = 0;	// stride
		p[5] = 0; p[6] = 0;	// glyph cell width
		memset(&p[7], 0, UV_UI_REMOTE_FONT_WIDTHS);
#if CONFIG_UI_OPENGL
		// The host backend measures every glyph when it builds its atlas, and
		// keeps what FreeType gave it: 26.6 fixed point, sixty-fourths of a
		// pixel. Everything else that reads ft_char[].advance divides by 64,
		// and so must this - the wire carries whole pixels.
		for (uint16_t i = 0; i < UV_UI_REMOTE_FONT_WIDTHS; i++) {
			int32_t adv = (int32_t) font->ft_char[i].advance / 64;
			p[7 + i] = (adv < 0) ? 0 : ((adv > 255) ? 255 : (uint8_t) adv);
		}
#elif CONFIG_FT81X
		// the widths are the first 128 bytes of the font's metric block,
		// whether that is the ROM one or a custom font's
		(void) uv_ft81x_font_widths(index, &p[7]);
#else
		// no per-glyph metrics on this backend; height alone still helps
#endif
		ret = UV_UI_REMOTE_FONT_BODY_HDR_LEN;
	}
	else {
	}
	return ret;
}


void uv_ui_remote_set_asset_provider(uv_ui_remote_asset_size_t size_callb,
		uv_ui_remote_asset_read_t read_callb) {
	this->asset_size_callb = size_callb;
	this->asset_read_callb = read_callb;
}


void uv_ui_remote_asset_requested(uint8_t kind, uint32_t id) {
	bool placed = false;
	// already queued: asking twice for the same thing is normal, the sink
	// re-asks until it has it
	for (uint8_t i = 0; (i < CONFIG_UI_REMOTE_ASSET_REQ_MAX) && !placed; i++) {
		if (this->wanted[i].pending &&
				(this->wanted[i].kind == kind) && (this->wanted[i].id == id)) {
			placed = true;
		}
		else {
		}
	}
	for (uint8_t i = 0; (i < CONFIG_UI_REMOTE_ASSET_REQ_MAX) && !placed; i++) {
		if (!this->wanted[i].pending) {
			this->wanted[i].kind = kind;
			this->wanted[i].id = id;
			this->wanted[i].pending = true;
			placed = true;
		}
		else {
		}
	}
	if (!placed) {
		// full: drop the oldest by overwriting slot 0. The sink asks again the
		// next time it draws something it is still missing, so nothing is lost
		// for good.
		this->wanted[0].kind = kind;
		this->wanted[0].id = id;
		this->wanted[0].pending = true;
	}
	else {
	}

	if (kind == UV_UI_REMOTE_ASSET_KIND_BITMAP) {
		// A bitmap can only be answered from inside the draw that uses it: that
		// is the one moment its file name is in reach. A screen that has
		// settled is not going to be drawn again by itself, so a request for
		// one of its images would sit in the queue for ever, and the sink would
		// keep looking at an outline where the icon belongs. Owing the sink a
		// screen is exactly what *missed* means, so say so and the next display
		// step draws one.
		this->missed = true;
	}
	else {
		// a font is built from ui_fonts[] and needs nothing drawn
	}
}


void uv_ui_remote_asset_cancel(uint8_t kind, uint32_t id) {
	for (uint8_t i = 0; i < CONFIG_UI_REMOTE_ASSET_REQ_MAX; i++) {
		if (this->wanted[i].pending &&
				(this->wanted[i].kind == kind) && (this->wanted[i].id == id)) {
			this->wanted[i].pending = false;
		}
		else {
		}
	}
	if (this->tx_asset.active &&
			(this->tx_asset.kind == kind) && (this->tx_asset.id == id)) {
		// stop mid-transfer: the sink sees a truncated asset and asks again
		this->tx_asset.active = false;
	}
	else {
	}
}


/// @brief: Takes the next request off the queue and opens a transfer for it.
/// An asset the provider cannot size is answered with an empty one, which tells
/// the sink to stop asking rather than leaving it to guess.
static void asset_start_next(void) {
	// a bitmap caught while it was being drawn goes first: its name is in hand
	if (this->pending_name != NULL) {
		this->tx_asset.kind = UV_UI_REMOTE_ASSET_KIND_BITMAP;
		this->tx_asset.id = this->pending_id;
		this->tx_asset.name = this->pending_name;
		this->pending_name = NULL;
		this->tx_asset.total = (this->asset_size_callb != NULL) ?
				this->asset_size_callb(UV_UI_REMOTE_ASSET_KIND_BITMAP,
						this->tx_asset.name) : 0;
		this->tx_asset.offset = 0;
		this->tx_asset.active = true;
	}
	else {
	}

	// a font can be answered without waiting to see it drawn, since it is
	// described by ui_fonts[] rather than by anything in storage
	for (uint8_t i = 0;
			(i < CONFIG_UI_REMOTE_ASSET_REQ_MAX) && !this->tx_asset.active; i++) {
		if (this->wanted[i].pending &&
				(this->wanted[i].kind == UV_UI_REMOTE_ASSET_KIND_FONT)) {
			this->tx_asset.kind = this->wanted[i].kind;
			this->tx_asset.id = this->wanted[i].id;
			this->tx_asset.name = NULL;
			this->wanted[i].pending = false;
			this->tx_asset.total = font_body_build(this->tx_asset.id);
			this->tx_asset.offset = 0;
			this->tx_asset.active = true;
		}
		else {
		}
	}
}


/// @brief: Fills the outgoing chunk buffer, if there is an asset to send and
/// the transport has taken the last one. Runs on the UI task at a frame
/// boundary: the provider reads external flash, which the UI task owns.
static void asset_prepare_chunk(void) {
	if (!this->tx_asset.ready) {
		if (!this->tx_asset.active) {
			asset_start_next();
		}
		else {
		}
		if (this->tx_asset.active) {
			uint16_t n = 0;
			uint8_t flags = 0;
			if (this->tx_asset.offset == 0) {
				// first chunk carries what the asset is and how long it is
				flags |= UV_UI_REMOTE_ASSET_FLAG_START;
				this->tx_asset.buf[0] = this->tx_asset.kind;
				this->tx_asset.buf[1] = (uint8_t) (this->tx_asset.id & 0xFFu);
				this->tx_asset.buf[2] = (uint8_t) ((this->tx_asset.id >> 8) & 0xFFu);
				this->tx_asset.buf[3] = (uint8_t) ((this->tx_asset.id >> 16) & 0xFFu);
				this->tx_asset.buf[4] = (uint8_t) ((this->tx_asset.id >> 24) & 0xFFu);
				this->tx_asset.buf[5] = (uint8_t) (this->tx_asset.total & 0xFFu);
				this->tx_asset.buf[6] = (uint8_t) ((this->tx_asset.total >> 8) & 0xFFu);
				this->tx_asset.buf[7] = (uint8_t) ((this->tx_asset.total >> 16) & 0xFFu);
				this->tx_asset.buf[8] = (uint8_t) ((this->tx_asset.total >> 24) & 0xFFu);
				n = UV_UI_REMOTE_ASSET_HDR_LEN;
			}
			else {
			}

			uint32_t left = this->tx_asset.total - this->tx_asset.offset;
			uint32_t want = (left > UV_UI_REMOTE_ASSET_CHUNK) ?
					UV_UI_REMOTE_ASSET_CHUNK : left;
			uint32_t got = 0;
			if (want == 0) {
			}
			else if (this->tx_asset.kind == UV_UI_REMOTE_ASSET_KIND_FONT) {
				memcpy(&this->tx_asset.buf[n],
						&this->font_body[this->tx_asset.offset], want);
				got = want;
			}
			else if ((this->asset_read_callb != NULL) &&
					(this->tx_asset.name != NULL)) {
				got = this->asset_read_callb(this->tx_asset.kind,
						this->tx_asset.name, this->tx_asset.offset,
						&this->tx_asset.buf[n], want);
			}
			else {
			}
			n = (uint16_t) (n + got);
			this->tx_asset.offset += got;

			// the provider running dry ends the transfer as surely as reaching
			// the total does, so a short read cannot leave it hanging
			if ((got < want) || (this->tx_asset.offset >= this->tx_asset.total)) {
				flags |= UV_UI_REMOTE_ASSET_FLAG_END;
				this->tx_asset.active = false;
			}
			else {
			}
			this->tx_asset.len = n;
			this->tx_asset.flags = flags;
			this->tx_asset.ready = true;
		}
		else {
		}
	}
	else {
	}
}


bool uv_ui_remote_asset_chunk_pending(const uint8_t **data, uint16_t *len,
		uint8_t *flags) {
	bool ret = false;
	if (this->enabled && this->tx_asset.ready) {
		*data = this->tx_asset.buf;
		*len = this->tx_asset.len;
		*flags = this->tx_asset.flags;
		ret = true;
	}
	else {
	}
	return ret;
}


void uv_ui_remote_asset_chunk_sent(void) {
	this->tx_asset.ready = false;
}


// --- encode hooks -----------------------------------------------------------

void uv_ui_remote_encode_clear(color_t c) {
	// WAIT is included on purpose. It means the transport has just finished and
	// we are waiting for a clean boundary to start capturing again - and a
	// clear IS that boundary, whatever the drawing was doing when the transport
	// finished. Resuming here is what keeps an ordinary screen from being lost
	// to that window at all; only a screen that never clears, which is what a
	// dialog draws, still has to wait for the swap.
	if (this->enabled &&
			((this->state == REMOTE_UI_IDLE) ||
			 (this->state == REMOTE_UI_GATHER) ||
			 (this->state == REMOTE_UI_WAIT))) {
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
				// frame did not fit; drop it and realign at the next clear.
				// The sink is now behind by a screen it will never be sent.
				this->state = REMOTE_UI_IDLE;
				this->missed = true;
			}
			else {
				uint32_t hash = fnv1a(this->buf, this->buf_len);
				// this screen was captured, so nothing is owed for it - even
				// when it turns out to be the one the sink already has
				this->missed = false;
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
			// boundary at which we may resume capturing the next frame. The
			// screen just drawn went past uncaptured.
			this->state = REMOTE_UI_IDLE;
			this->missed = true;
		}
		else if (this->state == REMOTE_UI_TX) {
			// a whole screen drawn while the transport was still draining the
			// last one: it could not be captured and is now owed
			this->missed = true;
		}
		else {
			// IDLE: a swap with nothing drawn, so nothing was missed
		}

		// A frame boundary is the one moment the UI task is not drawing, so it
		// is where an asset chunk is read out of flash. One per frame keeps the
		// reads off the critical path and off the transport task, which shares
		// the bus with nobody's permission.
		asset_prepare_chunk();
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
		uint32_t id = uv_ui_remote_bitmap_id(bitmap);

		// The sink asked for this one. Right here is the only place its file
		// name is in reach, so take it now; the frame boundary opens the
		// transfer.
		if ((this->pending_name == NULL) && (bitmap != NULL)) {
			for (uint8_t i = 0; i < CONFIG_UI_REMOTE_ASSET_REQ_MAX; i++) {
				if (this->wanted[i].pending &&
						(this->wanted[i].kind ==
								UV_UI_REMOTE_ASSET_KIND_BITMAP) &&
						(this->wanted[i].id == id)) {
					this->pending_name = bitmap->filename;
					this->pending_id = id;
					this->wanted[i].pending = false;
				}
				else {
				}
			}
		}
		else {
		}

		ap8((uint8_t) UV_UI_REMOTE_OP_BITMAP);
		ap32(id);
		ap16((uint16_t) x);
		ap16((uint16_t) y);
		ap16((uint16_t) w);
		ap16((uint16_t) h);
		ap32(wrap);
		ap32(color);
	}
}


#endif /* CONFIG_UI && CONFIG_UI_REMOTE */
