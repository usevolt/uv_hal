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

#include "ui/uv_uikeyboard.h"
#include <string.h>
#include "uv_rtos.h"

#if CONFIG_UI



/// @brief: A single character key.
///
/// The text is given as a string rather than a char: the Nordic letters are
/// two-byte UTF-8 sequences, which the draw layer translates into the font's
/// glyph slots and which are stored into the buffer as they are.
typedef struct {
	/// @brief: inserted and shown when the key is not shifted
	const char *normal;
	/// @brief: inserted and shown when the key is shifted
	const char *shifted;
	/// @brief: when true, the currently inactive alternative is drawn under the
	/// active one in a smaller, dimmed font. Set for the number row, whose
	/// shifted characters are the specials needed for URLs and which the digit
	/// alone gives no hint of.
	bool show_alt;
} keyb_key_st;

/// @brief: Lists all characters shown on the keyboard, line by line.
/// *space* is the only special character which is shown below these
static const keyb_key_st keys[] = {
		{"1", "/", true}, {"2", ":", true}, {"3", "\"", true},
		{"4", "?", true}, {"5", "=", true}, {"6", ".", true},
		{"7", "-", true}, {"8", "_", true}, {"9", "@", true},
		{"0", "&", true},
		{"q", "Q", false}, {"w", "W", false}, {"e", "E", false},
		{"r", "R", false}, {"t", "T", false}, {"y", "Y", false},
		{"u", "U", false}, {"i", "I", false}, {"o", "O", false},
		{"p", "P", false},
		{"a", "A", false}, {"s", "S", false}, {"d", "D", false},
		{"f", "F", false}, {"g", "G", false}, {"h", "H", false},
		{"j", "J", false}, {"k", "K", false}, {"l", "L", false},
		{"z", "Z", false}, {"x", "X", false}, {"c", "C", false},
		{"v", "V", false}, {"b", "B", false}, {"n", "N", false},
		{"m", "M", false}, {"ä", "Ä", false}, {"ö", "Ö", false}
};

#define KEY_COUNT			((int16_t) (sizeof(keys) / sizeof(keys[0])))

typedef struct {
	EXTENDS(uv_uidisplay_st);

	uv_uiobject_st *bfr[1];
	/// @brief: shift toggled on with the shift key
	bool shift;
	/// @brief: The first character of an empty buffer is upper case without
	/// the user touching shift. Kept apart from *shift* so that the number row
	/// still gives the digits, and not the shifted URL specials, at that point.
	bool autocaps;
	/// @brief: The key currently held down, drawn pressed. KEY_NONE when the
	/// screen is not touched.
	int16_t pressed;
	const char *title;
	char *buffer;
	const uv_uistyle_st *style;
} uv_uikeyboard_st;

static uv_uikeyboard_st *this;

/// @brief: Defines the length of each character line. e.g. How many buttons
/// are shown on each line
static const uint8_t line_lengths[] = {10, 10, 9, 9};

/// @brief: Defines the indent of each character line in half button widths.
/// The lines are laid out so that the wide key at the end of each of them
/// (backspace, enter, shift) ends at the right edge of the screen.
static const uint8_t line_indents[] = {0, 1, 2, 2};

/// @brief: defines the count of the lines on keyboard
/// @note: Last line is reserved for space
#define LINE_COUNT			5

/// @brief: Defines the height of the keyboard relative to the full display height
#define KEYBOARD_HEIGHT_PPT		750

/// @brief: Defines the button height
#define BUTTON_H			(LCD_HPPT(KEYBOARD_HEIGHT_PPT) / LINE_COUNT)
/// @brief: Defines the button width
/// @note: This is relative to the maximum line lengths!
#define BUTTON_W			(LCD_W_PX / 12)

#define BUTTONS_START		(LCD_HPPT((int32_t) 1000 - KEYBOARD_HEIGHT_PPT) - 1)



/// @brief: Key codes returned by get_press. Values from 0 to KEY_COUNT - 1
/// index keys[], these identify everything else on the screen.
enum {
	KEY_NONE = -1,
	KEY_SHIFT = 1000,
	KEY_BACKSPACE,
	KEY_ENTER,
	KEY_SPACE
};

static void update_input(char *input, const uv_uistyle_st *style);



/// @brief: Returns the line which the character key *index* belongs to, and
/// writes the index of the first key on that line to *line_start*.
static uint8_t key_line(int16_t index, int16_t *line_start) {
	uint8_t line = 0;
	int16_t start = 0;
	while ((line < (LINE_COUNT - 2)) &&
			(index >= (start + (int16_t) line_lengths[line]))) {
		start += (int16_t) line_lengths[line];
		line++;
	}
	*line_start = start;
	return line;
}



/// @brief: Calculates the bounding box of *key*, which is either an index to
/// keys[] or one of the KEY_* codes. Shared by the drawing and the touch
/// handling so that the two can never disagree on where a key is.
static void key_geometry(int16_t key, int16_t *x, int16_t *y,
		int16_t *w, int16_t *h) {
	uint8_t line;
	int16_t line_start = 0;

	if (key == KEY_SPACE) {
		line = LINE_COUNT - 1;
	}
	else if (key == KEY_BACKSPACE) {
		line = 0;
	}
	else if (key == KEY_ENTER) {
		line = 1;
	}
	else if (key == KEY_SHIFT) {
		line = 3;
	}
	else {
		line = key_line(key, &line_start);
	}

	*y = (int16_t) (BUTTONS_START + (BUTTON_H * (int16_t) line));
	*h = BUTTON_H;

	if (key == KEY_SPACE) {
		*x = LCD_WPPT(100);
		*w = LCD_WPPT(800);
	}
	else if ((key == KEY_BACKSPACE) ||
			(key == KEY_SHIFT)) {
		// the wide key follows the last character key of its line
		*x = (int16_t) (((BUTTON_W / 2) * (int16_t) line_indents[line]) +
				(BUTTON_W * (int16_t) line_lengths[line]));
		*w = BUTTON_W * 2;
	}
	else if (key == KEY_ENTER) {
		*x = (int16_t) (((BUTTON_W / 2) * (int16_t) line_indents[line]) +
				(BUTTON_W * (int16_t) line_lengths[line]));
		*w = (BUTTON_W * 3) / 2;
		// enter is two lines tall, reaching down over the line below it
		*h = (BUTTON_H * 2) + 1;
	}
	else {
		*x = (int16_t) (((BUTTON_W / 2) * (int16_t) line_indents[line]) +
				(BUTTON_W * (key - line_start)));
		*w = BUTTON_W;
	}
}



/// @brief: Returns the text which the character key *index* inserts in the
/// current shift state.
static const char *key_text(int16_t index) {
	const keyb_key_st *key = &keys[index];
	// The automatic upper case of the first character applies to the letters
	// only: on the number row shift picks the URL special instead, and nobody
	// asks for that by just opening the keyboard.
	bool shifted = key->show_alt ? this->shift : (this->shift || this->autocaps);
	return shifted ? key->shifted : key->normal;
}



/// @brief: Returns the font used for the dimmed secondary character, one step
/// smaller than the style's font when the style leaves room for one.
static uv_font_st *alt_font(void) {
	uv_font_st *ret = this->style->font;
	for (uint8_t i = 1; i < UI_MAX_FONT_COUNT; i++) {
		if (this->style->font == &ui_fonts[i]) {
			ret = &ui_fonts[i - 1];
			break;
		}
	}
	return ret;
}



/// @brief: Draws a single key. *alt* is the dimmed secondary character drawn
/// under *text*, or NULL for the keys that have none. *active* marks a key
/// that is toggled on (shift).
///
/// A pressed key is drawn like a pressed uv_uibutton: the fill is brightened
/// and the light and shadow edges swap, which makes it look pushed in.
static void draw_key(int16_t key, const char *text, const char *alt,
		bool active) {
	int16_t x, y, w, h;
	key_geometry(key, &x, &y, &w, &h);
	bool pressed = (this->pressed == key);
	color_t main_c = active ?
			uv_uic_brighten(this->style->bg_c, 80) : this->style->bg_c;
	color_t bg_c = pressed ? uv_uic_brighten(main_c, 20) : main_c;
	color_t light_c = uv_uic_brighten(main_c, pressed ? -80 : 80);
	color_t shadow_c = uv_uic_brighten(main_c, pressed ? 80 : -80);

	uv_ui_draw_shadowrrect(x, y, w, h, CONFIG_UI_RADIUS,
			bg_c, light_c, shadow_c);
	if (alt == NULL) {
		uv_ui_draw_string((char*) text, this->style->font,
				x + (w / 2), y + (h / 2),
				ALIGN_CENTER, this->style->text_color);
	}
	else {
		uv_ui_draw_string((char*) text, this->style->font,
				x + (w / 2), y + ((h * 35) / 100),
				ALIGN_CENTER, this->style->text_color);
		uv_ui_draw_string((char*) alt, alt_font(),
				x + (w / 2), y + ((h * 78) / 100),
				ALIGN_CENTER, uv_uic_brighten(this->style->text_color, -80));
	}
}



static void draw(void *me, const uv_bounding_box_st *pbb) {
	// background
	uv_ui_clear(this->style->window_c);

	uv_ui_draw_string((char*) this->title, this->style->font,
			LCD_WPPT(500), 0, ALIGN_TOP_CENTER, this->style->text_color);

	// draw current text
	update_input(this->buffer, this->style);

	// draw character buttons
	for (int16_t i = 0; i < KEY_COUNT; i++) {
		const char *text = key_text(i);
		const char *alt = NULL;
		if (keys[i].show_alt) {
			// the alternative of whichever character the key inserts now
			alt = (text == keys[i].normal) ? keys[i].shifted : keys[i].normal;
		}
		draw_key(i, text, alt, false);
	}

	// draw the wide keys
	draw_key(KEY_BACKSPACE, "Backspace", NULL, false);
	draw_key(KEY_ENTER, "Enter", NULL, false);
	draw_key(KEY_SHIFT, "Shift", NULL, this->shift || this->autocaps);
	draw_key(KEY_SPACE, "Space", NULL, false);

	// draw touch indicator
	uv_uidisplay_draw_touch_ind(this);

	// update the ft81x display
	uv_ui_dlswap();
}



/// @brief: Returns true if (*tx*, *ty*) is inside *key*
static bool key_hit(int16_t key, int16_t tx, int16_t ty) {
	int16_t x, y, w, h;
	key_geometry(key, &x, &y, &w, &h);
	return ((tx >= x) && (tx < (x + w)) &&
			(ty >= y) && (ty < (y + h)));
}



/// @brief: Returns the key at (*tx*, *ty*), or KEY_NONE if the point hits none
static int16_t key_at(int16_t tx, int16_t ty) {
	static const int16_t wide_keys[] = {
			KEY_BACKSPACE, KEY_ENTER, KEY_SHIFT, KEY_SPACE
	};
	int16_t ret = KEY_NONE;
	for (int16_t i = 0; i < KEY_COUNT; i++) {
		if (key_hit(i, tx, ty)) {
			ret = i;
			break;
		}
	}
	if (ret == KEY_NONE) {
		for (uint8_t i = 0; i < (sizeof(wide_keys) / sizeof(wide_keys[0])); i++) {
			if (key_hit(wide_keys[i], tx, ty)) {
				ret = wide_keys[i];
				break;
			}
		}
	}
	return ret;
}



/// @brief: Parses the press action and returns the key pressed. Takes also care
/// of touch release events and of keeping the key held down drawn pressed.
///
/// @return: The key clicked, or KEY_NONE when no key was clicked
static int16_t get_press(uv_touch_st *touch) {
	int16_t ret = KEY_NONE;
	int16_t down = KEY_NONE;

	if ((touch->action == TOUCH_PRESSED) ||
			(touch->action == TOUCH_IS_DOWN) ||
			(touch->action == TOUCH_CLICKED)) {
		down = key_at(touch->x, touch->y);
	}
	if (touch->action == TOUCH_CLICKED) {
		// the press ended on this key: report it and let go of the highlight
		ret = down;
		down = KEY_NONE;
	}
	if (this->pressed != down) {
		this->pressed = down;
		uv_ui_refresh(this);
	}

	return ret;
}


static void update_input(char *input, const uv_uistyle_st *style) {
	// clear all previous texts
	uv_ui_draw_string(input, style->font, LCD_WPPT(500), style->font->char_height,
			ALIGN_TOP_CENTER, style->text_color);

}



bool uv_uikeyboard_show(const char *title, char *buffer,
		uint16_t buf_len, const uv_uistyle_st *style) {
	bool ret;
	uv_uikeyboard_st me;
	this = &me;
	uv_uidisplay_init(this, this->bfr, &uv_uistyles[0]);
	uv_uiobject_set_draw_callb(this, &draw);
	this->shift = false;
	this->autocaps = true;
	this->pressed = KEY_NONE;
	this->title = title;
	this->buffer = buffer;
	this->style = style;
	bool nullterm = false;
	for (uint8_t i = 0; i < buf_len; i++) {
		if (buffer[i] == '\0') {
			nullterm = true;
			break;
		}
	}
	if (!nullterm) {
		// buffer doesn't contain a null-terminated string,
		// initialize it to zero-length string
		buffer[0] = '\0';
	}
	uint16_t input_len = strlen(buffer);

	while (true) {
		uint16_t step_ms = 20;
		uv_uidisplay_step(this, step_ms);

		int16_t key = get_press(uv_uidisplay_get_touch(this));
		if (key != KEY_NONE) {
			if (key == KEY_SHIFT) {
				if (this->autocaps) {
					// the automatic upper case is what the user sees as shift
					// being on, so this is the press that turns it off
					this->autocaps = false;
					this->shift = false;
				}
				else {
					this->shift = !this->shift;
				}
			}
			else if (key == KEY_ENTER) {
				// replace added new lines with spaces
				for (int16_t i = 0; i < strlen(buffer); i++) {
					if (buffer[i] == '\n') buffer[i] = ' ';
				}
				ret = input_len ? true : false;
				break;
			}
			else if (key == KEY_BACKSPACE) {
				if (input_len) {
					// a character can be several bytes of UTF-8 (ä, ö), and
					// leaving half of a sequence behind would render as garbage
					input_len--;
					while ((input_len > 0) &&
							(((uint8_t) this->buffer[input_len] & 0xC0u) == 0x80u)) {
						input_len--;
					}
				}
				else {
					// first character defaults to uppercase
					this->autocaps = true;
				}
				this->buffer[input_len] = '\0';
				update_input(this->buffer, this->style);
			}
			// normal character pressed
			else {
				const char *str = (key == KEY_SPACE) ? " " : key_text(key);
				uint16_t len = strlen(str);
				if ((input_len + len) < buf_len) {
					memcpy(&this->buffer[input_len], str, len);
					input_len += len;
					this->buffer[input_len] = '\0';
					this->autocaps = false;
					update_input(this->buffer, this->style);
				}
			}
		}

		uv_rtos_task_delay(step_ms);
	}

	return ret;
}





#endif
