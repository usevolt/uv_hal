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


#ifndef UV_HAL_INC_UI_UV_UIDIGITEDIT_H_
#define UV_HAL_INC_UI_UV_UIDIGITEDIT_H_

#include "uv_utilities.h"
#include "uv_ui.h"


#if CONFIG_UI


#ifndef CONFIG_UI_DIGITEDIT_INCDEC_BUTTON_WIDTH
#warning "CONFIG_UI_DIGITEDIT_INCDEC_BUTTON_WIDTH should define the width of \
the + and - buttons in pixels"
#define CONFIG_UI_DIGITEDIT_INCDEC_BUTTON_WIDTH	40
#endif


typedef enum {
	UIDIGITEDIT_MODE_NORMAL = 0,
	// INCDEC mode with plus & minus buttons
	UIDIGITEDIT_MODE_INCDEC,
	// RODIGIT mode with bare digit shown without frames or other graphic
	UIDIGITEDIT_MODE_RODIGIT,
	UIDIGITEDIT_MODE_COUNT
} uv_uidigitedit_mode_e;



/// @brief: Structure for showing text label on the screen.
typedef struct __attribute__((packed)) {
	EXTENDS(uv_uilabel_st);

	char str[16];
	char *title;
	char *unit;
	color_t bg_color;
	int32_t value;
	uint16_t divider;
	bool changed;
	const uv_uistyle_st *style;
	int32_t limit_max;
	int32_t limit_min;
	uv_uidigitedit_mode_e mode;
	union {
		struct {
			char *numpaddialog_title;
		} normal;
		struct {
			int8_t pressed_button;
			int16_t inc_step;
			uv_delay_st delay;
		} incdec;
		struct {
			uv_font_st *title_font;
		} rodigit;
	} modedata;
} uv_uidigitedit_st;


#undef this
#define this ((uv_uidigitedit_st*)me)


void uv_uidigitedit_init(void *me, int32_t value, const uv_uistyle_st *style);


/// @brief: Set's the text of all objects which are inherited from uv_uilabel_st
///
/// @note: Since string are passed as a reference parameters, updating the same text
/// will not refresh the label itself. Thus uv_ui_refresh should be called after updating the text.
void uv_uidigitedit_set_value(void *me, int32_t value);




/// @brief: Sets the divider. When set, the shown value will be divided by *value*,
/// and the decimals are shown after a period mark. Note that the *value* should
/// be dividable by 10.
void uv_uidigitedit_set_divider(void *me, uint16_t value);


static inline uint16_t uv_uidigitedit_get_divider(void *me) {
	return this->divider;
}


void uv_uidigitedit_set_unit(void *me, char *value);

static inline char *uv_uidigitedit_get_unit(void *me) {
	return this->unit;
}

/// @brief: Sets the color of the label text
static inline void uv_uidigitedit_set_text_color(void *me, color_t c) {
	uv_uilabel_set_color(me, c);
}

static inline void uv_uidigitedit_set_bg_color(void *me, color_t c) {
	this->bg_color = c;
}


/// @brief: In incdec mode, + and - buttons are shown on right and left side of the
/// value that can be pressed.
void uv_uidigitedit_set_mode(void *me, uv_uidigitedit_mode_e value);


static inline uv_uidigitedit_mode_e uv_uidigitedit_get_mode(void *me) {
	return this->mode;
}


/// @brief: Inc step is used in UIDIGITEDIT_MODE_INCDEC
static inline void uv_uidigitedit_set_inc_step(void *me, int16_t value) {
	this->modedata.incdec.inc_step = value;
}

static inline int16_t uv_uidigitedit_get_inc_step(void *me) {
	return this->modedata.incdec.inc_step;
}

/// @brief: Sets the title for the digitedit. The title text is shown below the digitedit fiel
static inline void uv_uidigitedit_set_title(void *me, char *value) {
	this->title = value;
}

static inline char *uv_uidigitedit_get_title(void *me) {
	return this->title;
}

/// @brief: Sets the string for the numpad dialog which opens when the uidigitedit is clicked
static inline void uv_uidigitedit_set_numpad_title(void *me, char *value) {
	this->modedata.normal.numpaddialog_title = value;
}

/// @brief: Returns true on the step cycle when the value was changed
static inline bool uv_uidigitedit_value_changed(void *me) {
	return this->changed;
}

static inline int64_t uv_uidigitedit_get_value(void *me) {
	return this->value;
}


/// @brief: Sets the maximum limit
static inline void uv_uidigitedit_set_maxlimit(void *me, int32_t value) {
	this->limit_max = value;
}

/// @brief: Returns the maximum limit
static inline int32_t uv_uidigitedit_get_maxlimit(void *me) {
	return this->limit_max;
}

/// @brief: Sets the negative limit. The limit defaults to 0, in which case
/// the uinumpad wont display sign-button.
static inline void uv_uidigitedit_set_minlimit(void *me, int32_t value) {
	this->limit_min = value;
}

/// @brief: Sets both limits at once
static inline void uv_uidigitedit_set_limits(void *me, int32_t min_value, int32_t max_value) {
	this->limit_min = min_value;
	this->limit_max = max_value;
}


static inline void uv_uidigitedit_set_font(void *me, uv_font_st *font) {
	uv_uilabel_set_font(me, font);
}

static inline uv_font_st *uv_uidigitedit_get_font(void *me) {
	return uv_uilabel_get_font(me);
}


/// @brief Sets the title font. Only available for type RODIGIT
static inline void uv_uidigitedit_rodigit_set_title_font(void *me, uv_font_st *font) {
	this->modedata.rodigit.title_font = font;
}

static inline uv_font_st *uv_uidigitedit_rodigit_get_title_font(void *me) {
	return this->modedata.rodigit.title_font;
}

void uv_uidigitedit_draw(void *me, const uv_bounding_box_st *pbb);



#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIDIGITEDIT_H_ */
