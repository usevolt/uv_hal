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
#if CONFIG_TARGET_LINUX
// defined below, used by uv_uitextedit_set_text further up
static void cursor_clamp(uv_uitextedit_st *this_);
#endif
static void touch(void *me, uv_touch_st *touch);


// Longest prefix of a value that is masked or measured in one go. Only affects
// very long values, and then only cosmetically.
#define UITEXTEDIT_MASK_MAX		128


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
	this->cursor = 0;
#if CONFIG_UI_ENABLEFOCUS
	// a text field is the archetypal focus target: focused means the keyboard
	// is typing into this one
	uv_uiobject_set_enablefocus(this, true);
#endif
	this->was_touched = false;
	this->blink_ms = 0;
	this->submitted = false;
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
#if CONFIG_TARGET_LINUX
	cursor_clamp(this);
#endif
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
#if CONFIG_UI_ENABLEFOCUS
	if (uv_uiobject_get_focused(this)) {
		// around the entry box only: the title below it is not typed into
		uv_uiobject_draw_focus(x, y, uv_uibb(this)->width, height,
				uv_uic_brighten(this->bg_color, 120));
	}
#endif

	// password fields render each character as '*' instead of in the clear; the
	// buffer itself keeps the real text. The mask is sized to the visible length,
	// capped so a very long value cannot overflow the local buffer (cosmetic only).
	char *drawstr = this->buffer;
	char maskbuf[UITEXTEDIT_MASK_MAX];
	if ((this->flags & UITEXTEDIT_FLAG_PASSWORD) != 0) {
		size_t n = strlen(this->buffer);
		if (n >= sizeof(maskbuf)) {
			n = sizeof(maskbuf) - 1;
		}
		memset(maskbuf, '*', n);
		maskbuf[n] = '\0';
		drawstr = maskbuf;
	}

	// honor the (inherited) label alignment: left-aligned fields draw the text at
	// a small left padding, otherwise it is centered
	alignment_e al = ((uv_uilabel_st *) this)->align;
	bool leftalign = ((al & UI_HALIGN_MASK) == UI_HALIGN_LEFT);
	int16_t textcy = y + height / 2 + CONFIG_UI_RADIUS;
	int16_t textx = leftalign ?
			(x + TITLE_OFFSET) : (x + uv_uibb(this)->width / 2);
	alignment_e stral = leftalign ? ALIGN_CENTER_LEFT : UI_ALIGN_CENTER;
	uv_ui_draw_string(drawstr, font, textx, textcy, stral, text_color);

#if CONFIG_TARGET_LINUX
	if (this->editing && this->blink_ms < UITEXTEDIT_CURSOR_BLINK_MS) {
		int16_t text_w = uv_ui_get_string_width(drawstr, font);
		// width of the text before the caret. Measured on the string actually
		// drawn, so a password field measures its mask rather than the value
		// hiding behind it.
		char head[UITEXTEDIT_MASK_MAX];
		uint16_t n = this->cursor;
		if (n >= sizeof(head)) {
			n = sizeof(head) - 1;
		}
		memcpy(head, drawstr, n);
		head[n] = '\0';
		int16_t head_w = uv_ui_get_string_width(head, font);
		// the text starts at the left edge when left aligned, and half its
		// width to the left of the centre otherwise
		int16_t text_x = leftalign ?
				(x + TITLE_OFFSET) :
				(x + uv_uibb(this)->width / 2 - text_w / 2);
		uv_ui_draw_string("|", font, text_x + head_w + 1, textcy, stral,
				text_color);
	}
#endif

	if (this->title) {
		uv_ui_draw_string(this->title, font,
				x + uv_uibb(this)->width / 2, y + height + TITLE_OFFSET,
				ALIGN_TOP_CENTER, text_color);
	}
}


#if CONFIG_TARGET_LINUX
/// @brief: Keeps the caret inside the text. Called after anything that can
/// change the buffer from outside, since the caret is an index into it.
static void cursor_clamp(uv_uitextedit_st *this_) {
	uint16_t len = strlen(this_->buffer);
	if (this_->cursor > len) {
		this_->cursor = len;
	}
}


/// @brief: Inserts one character at the caret and steps it past it.
/// @return: false when the buffer is full.
static bool cursor_insert(uv_uitextedit_st *this_, char c) {
	bool ret = false;
	uint16_t len = strlen(this_->buffer);
	if ((uint16_t) (len + 1) < this_->buf_len) {
		// shift the tail, terminator included, to open one slot
		memmove(&this_->buffer[this_->cursor + 1], &this_->buffer[this_->cursor],
				(size_t) (len - this_->cursor + 1));
		this_->buffer[this_->cursor] = c;
		this_->cursor++;
		ret = true;
	}
	return ret;
}


/// @brief: Removes the character before the caret (backspace).
static bool cursor_erase_left(uv_uitextedit_st *this_) {
	bool ret = false;
	if (this_->cursor > 0) {
		uint16_t len = strlen(this_->buffer);
		memmove(&this_->buffer[this_->cursor - 1], &this_->buffer[this_->cursor],
				(size_t) (len - this_->cursor + 1));
		this_->cursor--;
		ret = true;
	}
	return ret;
}


/// @brief: Removes the character at the caret (delete).
static bool cursor_erase_right(uv_uitextedit_st *this_) {
	bool ret = false;
	uint16_t len = strlen(this_->buffer);
	if (this_->cursor < len) {
		memmove(&this_->buffer[this_->cursor], &this_->buffer[this_->cursor + 1],
				(size_t) (len - this_->cursor));
		ret = true;
	}
	return ret;
}
#endif


static uv_uiobject_ret_e step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	this->changed = false;

#if CONFIG_TARGET_LINUX
	bool cmdline = (this->flags & UITEXTEDIT_FLAG_CMDLINE) != 0;
	this->submitted = false;
#if CONFIG_UI_ENABLEFOCUS
	// Editing follows the focus. Without this the field Tab moved away from
	// would keep its cursor blinking and, worse, keep draining the key queue -
	// swallowing the next Tab before the display could act on it.
	// Command-line fields are focused by their owner, not by the traversal.
	if (!cmdline &&
			(this->editing != uv_uiobject_get_focused(this))) {
		this->editing = uv_uiobject_get_focused(this);
		this->blink_ms = 0;
		if (this->editing) {
			// start at the end of the value, which is where typing continues
			this->cursor = strlen(this->buffer);
			// drop anything typed before this field took the focus
			while (uv_ui_get_key_press() != '\0') { }
		}
		else {
		}
		uv_ui_refresh(this);
	}
	else {
	}
#endif
	if (this->editing) {
		// blink the cursor, refreshing on every phase boundary. In command-line
		// mode the cursor is kept steady (blink_ms stays 0): a persistent command
		// line is often shown over a full-screen console, where a twice-a-second
		// full-display refresh for the blink is wasteful.
		if (!cmdline) {
			uint16_t prev_phase = this->blink_ms / UITEXTEDIT_CURSOR_BLINK_MS;
			this->blink_ms += step_ms;
			if (this->blink_ms >= UITEXTEDIT_CURSOR_BLINK_MS * 2) {
				this->blink_ms = 0;
			}
			uint16_t new_phase = this->blink_ms / UITEXTEDIT_CURSOR_BLINK_MS;
			if (prev_phase != new_phase) {
				uv_ui_refresh(this);
			}
		}

		// drain typed characters. Tab is left in the queue: the display owns it
		// and uses it to move the focus on.
		char c;
		while (
#if CONFIG_UI_ENABLEFOCUS
				(uv_ui_peek_key_press() != '\t') &&
#endif
				((c = uv_ui_get_key_press()) != '\0')) {
			if (c == '\n' || c == '\r') {
				if (cmdline) {
					// submit but keep focus; the owner reads the line
					this->submitted = true;
				}
				else {
					this->editing = false;
					this->changed = true;
				}
				uv_ui_refresh(this);
				break;
			}
			else if (c == '\b') {
				if (cursor_erase_left(this)) {
					uv_ui_refresh(this);
				}
			}
			else if (c == 0x7f) {
				// delete removes what is under the caret, backspace what is
				// before it
				if (cursor_erase_right(this)) {
					uv_ui_refresh(this);
				}
			}
			else if ((c == UI_KEY_LEFT) ||
					(c == UI_KEY_RIGHT) ||
					(c == UI_KEY_HOME) ||
					(c == UI_KEY_END)) {
				uint16_t len = strlen(this->buffer);
				uint16_t prev = this->cursor;
				if ((c == UI_KEY_LEFT) && (this->cursor > 0)) {
					this->cursor--;
				}
				else if ((c == UI_KEY_RIGHT) && (this->cursor < len)) {
					this->cursor++;
				}
				else if (c == UI_KEY_HOME) {
					this->cursor = 0;
				}
				else if (c == UI_KEY_END) {
					this->cursor = len;
				}
				else {
				}
				if (this->cursor != prev) {
					// show the caret straight away rather than mid-blink, so
					// holding an arrow key does not look like it is stuttering
					this->blink_ms = 0;
					uv_ui_refresh(this);
				}
				else {
				}
			}
			else if (c == 0x1b) {
				// ESC: blur (single "blur commits" path). Ignored in command-line
				// mode, where focus is owned by the application.
				if (!cmdline) {
					this->editing = false;
					this->changed = true;
					uv_ui_refresh(this);
					break;
				}
			}
			else if (c == UI_KEY_PASTE) {
				// Append what the clipboard holds, as far as it fits. A newline
				// would submit or blur the field if it were typed, so a pasted
				// one is dropped rather than acted on: pasting is meant to fill
				// the field in, not to activate it. A one-line field takes the
				// first line only, which is what pasting a copied row of text
				// into an address or name field should do.
				const char *paste = uv_ui_get_clipboard();
				bool added = false;
				for (uint16_t i = 0; paste[i] != '\0'; i++) {
					char pc = paste[i];
					if ((pc == '\n') ||
							(pc == '\r')) {
						if ((this->flags & UITEXTEDIT_FLAG_ONELINE) != 0) {
							break;
						}
						else {
							continue;
						}
					}
					else if ((pc < 0x20) ||
							(pc >= 0x7f)) {
						// tabs and other control bytes have no meaning here
						continue;
					}
					else {
					}
					if (!cursor_insert(this, pc)) {
						// buffer full
						break;
					}
					else {
					}
					added = true;
				}
				if (added) {
					uv_ui_refresh(this);
				}
				else {
				}
			}
			else if (c >= 0x20 && c < 0x7f) {
				if (cursor_insert(this, c)) {
					uv_ui_refresh(this);
				}
			}
			else {
				// ignore other control characters
			}
		}

		// click-outside detection (rising edge on touch). In command-line mode the
		// field is never blurred by a click; its focus is app-controlled.
		if (!cmdline) {
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
#if CONFIG_TARGET_LINUX
	// command-line fields are focused by the application, not by clicks
	if ((this->flags & UITEXTEDIT_FLAG_CMDLINE) != 0) {
		return;
	}
#endif
	if (touch->action == TOUCH_CLICKED) {
		touch->action = TOUCH_NONE;
#if CONFIG_TARGET_LINUX
#if CONFIG_UI_ENABLEFOCUS
		// clicking is the other way of moving the focus; take it from whoever
		// had it so only one field is ever editing
		uv_uiwindow_set_focus(this);
#endif
		this->editing = true;
		this->cursor = strlen(this->buffer);
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
