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

#ifndef UV_HAL_INC_UV_UI_REMOTE_H_
#define UV_HAL_INC_UV_UI_REMOTE_H_

#include <uv_hal_config.h>
#include <stdint.h>
#include <stdbool.h>

/// @file: Remote UI mirroring encoder.
///
/// Serializes the HAL uv_ui_draw_* commands into a compact binary command
/// stream so that another device can rebuild the identical display. This
/// module is the SOURCE side only: it captures one frame at a time into a
/// single buffer and hands it to the transport (uvcan REMOTE module) which
/// pulls the bytes with uv_ui_remote_frame_pending() / uv_ui_remote_frame_sent().
///
/// This header is included at the end of uv_ui_common.h (after the UI types are
/// defined). The encode functions are called from the public uv_ui_draw_*
/// wrappers in uv_ui_common.c.

#if !defined(CONFIG_UI_REMOTE)
#define CONFIG_UI_REMOTE			0
#endif

#if CONFIG_UI_REMOTE

/// @brief: Size of the single frame command buffer in bytes. A frame that
/// exceeds this is dropped (never truncated / partially sent).
#if !defined(CONFIG_UI_REMOTE_BUFFER_SIZE)
#define CONFIG_UI_REMOTE_BUFFER_SIZE	4096
#endif

/// @brief: Maximum number of distinct bitmap assets that can be registered and
/// referenced by a compact id on the wire.
#if !defined(CONFIG_UI_REMOTE_ASSET_MAX)
#define CONFIG_UI_REMOTE_ASSET_MAX		32
#endif

/// @brief: Wire value returned for an unknown / unregisterable bitmap or font.
#define UV_UI_REMOTE_ASSET_INVALID		0xFFFFu
#define UV_UI_REMOTE_FONT_UNKNOWN		0xFFu


/// @brief: Command opcodes of the compact UI command stream.
/// All multi-byte fields are little-endian. Coordinates are int16, sizes /
/// radii / widths are uint16, colors are 4-byte ARGB8888. A frame is the run
/// from FRAME_BEGIN (= uv_ui_clear) up to and including FRAME_END (= uv_ui_dlswap).
///
///   FRAME_BEGIN (5):  [op:1][color:4]
///   BITMAP     (19):  [op:1][bitmap_id:2][x:2][y:2][w:2][h:2][wrap:4][color:4]
///   POINT      (11):  [op:1][x:2][y:2][diameter:2][color:4]
///   RRECT      (15):  [op:1][x:2][y:2][w:2][h:2][radius:2][color:4]
///   LINE       (15):  [op:1][sx:2][sy:2][ex:2][ey:2][width:2][color:4]
///   LINESTRIP (10+4N): [op:1][strip_type:1][width:2][color:4][count:2][(x:2,y:2)*N]
///   POLYGON    (7+4N): [op:1][color:4][count:2][(x:2,y:2)*N]
///   STRING    (14+L):  [op:1][font_id:1][x:2][y:2][align:2][color:4][str_len:2][utf8:L]
///   MASK        (9):  [op:1][x:2][y:2][w:2][h:2]
///   FRAME_END   (1):  [op:1]
///
/// String payload is raw UTF-8 (not private glyph slots) so a non-uv_hal sink
/// can render it. font_id: bit7 = mono flag, bits0-6 = index into
/// ui_fonts[] / ui_mono_fonts[]; UV_UI_REMOTE_FONT_UNKNOWN if unresolved.
typedef enum {
	UV_UI_REMOTE_OP_FRAME_BEGIN = 0x01,
	UV_UI_REMOTE_OP_BITMAP      = 0x02,
	UV_UI_REMOTE_OP_POINT       = 0x03,
	UV_UI_REMOTE_OP_RRECT       = 0x04,
	UV_UI_REMOTE_OP_LINE        = 0x05,
	UV_UI_REMOTE_OP_LINESTRIP   = 0x06,
	UV_UI_REMOTE_OP_POLYGON     = 0x07,
	UV_UI_REMOTE_OP_STRING      = 0x08,
	UV_UI_REMOTE_OP_MASK        = 0x09,
	UV_UI_REMOTE_OP_FRAME_END   = 0x0A
} uv_ui_remote_op_e;


/// @brief: Reverse input action byte (sink -> source). The sink reports raw
/// press / release; this device's existing uv_uidisplay_step gesture state
/// machine derives DRAG / CLICK from the stream.
typedef enum {
	UV_UI_REMOTE_INPUT_RELEASE = 0,
	UV_UI_REMOTE_INPUT_PRESS   = 1
} uv_ui_remote_input_action_e;



/// @brief: Initializes the remote UI encoder (disabled until a sink connects).
void uv_ui_remote_init(void);

/// @brief: Drops any in-progress frame and clears the reverse-input latch and
/// asset registry.
void uv_ui_remote_reset(void);

/// @brief: Housekeeping hook (currently a no-op). Kept for lifecycle symmetry.
void uv_ui_remote_step(uint16_t step_ms);

/// @brief: Enables / disables mirroring. The transport enables this when a
/// remote sink is connected and disables it otherwise. While disabled all
/// encode_* calls are no-ops.
void uv_ui_remote_set_enabled(bool enabled);

/// @brief: True when mirroring is active (a sink is connected).
bool uv_ui_remote_active(void);

/// @brief: Assigns / returns the compact wire id of a bitmap asset.
/// Returns UV_UI_REMOTE_ASSET_INVALID if the registry is full or m is NULL.
uint16_t uv_ui_remote_register_bitmap(uv_uimedia_st *bitmap);


// --- transport pull interface (called by the uvcan REMOTE module) -----------

/// @brief: Returns true and points *data / *len at the completed frame buffer
/// when a frame is ready to transmit. The buffer is owned by the encoder and is
/// stable until uv_ui_remote_frame_sent() is called.
bool uv_ui_remote_frame_pending(const uint8_t **data, uint16_t *len);

/// @brief: Called by the transport once the whole pending frame has been
/// written to the link. Releases the buffer; gathering of the next frame
/// resumes at the following frame boundary (dlswap).
void uv_ui_remote_frame_sent(void);


// --- reverse input (sink -> source) -----------------------------------------

/// @brief: Injects a remote input event. Called from the transport rx handler.
void uv_ui_remote_input_inject(uint8_t action, int16_t x, int16_t y,
		int16_t scroll, char key);

/// @brief: Returns the latched remote touch state. Consumed by the public
/// uv_ui_get_touch() wrapper.
bool uv_ui_remote_get_touch(int16_t *x, int16_t *y);

/// @brief: Returns and clears accumulated remote scroll notches.
int16_t uv_ui_remote_get_scroll(void);

/// @brief: Returns and clears the last remote key press ('\0' if none).
char uv_ui_remote_get_key(void);


// --- encode hooks (called from the uv_ui_draw_* wrappers in uv_ui_common.c) --

void uv_ui_remote_encode_clear(color_t c);
void uv_ui_remote_encode_frame_end(void);
void uv_ui_remote_encode_point(int16_t x, int16_t y, color_t color, uint16_t diameter);
void uv_ui_remote_encode_rrect(int16_t x, int16_t y, uint16_t w, uint16_t h,
		uint16_t radius, color_t color);
void uv_ui_remote_encode_line(int16_t start_x, int16_t start_y,
		int16_t end_x, int16_t end_y, uint16_t width, color_t color);
void uv_ui_remote_encode_linestrip(const uv_ui_linestrip_point_st *points,
		uint16_t point_count, uint16_t line_width, color_t color,
		uv_ui_strip_type_e type);
void uv_ui_remote_encode_polygon(const uv_ui_linestrip_point_st *points,
		uint16_t point_count, color_t color);
void uv_ui_remote_encode_string(char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color);
void uv_ui_remote_encode_mask(int16_t x, int16_t y, int16_t width, int16_t height);
void uv_ui_remote_encode_bitmap(uv_uimedia_st *bitmap, int16_t x, int16_t y,
		int16_t w, int16_t h, uint32_t wrap, color_t color);


#endif /* CONFIG_UI_REMOTE */

#endif /* UV_HAL_INC_UV_UI_REMOTE_H_ */
