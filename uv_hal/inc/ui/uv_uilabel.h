/*
 * uv_label.h
 *
 *  Created on: Sep 30, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UILABEL_H_
#define UV_HAL_INC_UI_UV_UILABEL_H_


#include <ui/uv_uiobject.h>
#include <uv_hal_config.h>
#include <stdlib.h>
#include "uv_utilities.h"
#include "ui/uv_uifont.h"
#include <string.h>


#if CONFIG_LCD

/* Alignments */
enum {
	ALIGN_H_LEFT 			= (1 << 0),
	ALIGN_H_CENTER 			= (1 << 1),
	ALIGN_H_RIGHT 			= (1 << 2),
	ALIGN_V_TOP 			= (1 << 3),
	ALIGN_V_CENTER 			= (1 << 4),
	ALIGN_V_BOTTOM 			= (1 << 5),
	ALIGN_TOP_LEFT 			= (ALIGN_V_TOP | ALIGN_H_LEFT),
	ALIGN_CENTER_LEFT 		= (ALIGN_V_CENTER | ALIGN_H_LEFT),
	ALIGN_BOTTOM_LEFT 		= (ALIGN_V_BOTTOM | ALIGN_H_LEFT),
	ALIGN_TOP_CENTER 		= (ALIGN_V_TOP | ALIGN_H_CENTER),
	ALIGN_CENTER 			= (ALIGN_V_CENTER | ALIGN_H_CENTER),
	ALIGN_BOTTOM_CENTER 	= (ALIGN_V_BOTTOM | ALIGN_H_CENTER),
	ALIGN_TOP_RIGHT 		= (ALIGN_V_TOP | ALIGN_H_RIGHT),
	ALIGN_CENTER_RIGHT 		= (ALIGN_V_CENTER | ALIGN_H_RIGHT),
	ALIGN_BOTTOM_RIGHT 		= (ALIGN_V_BOTTOM | ALIGN_H_RIGHT)
};
typedef uint8_t alignment_e;





/// @brief: Structure for showing text label on the screen.
typedef struct {
	EXTENDS(uv_uiobject_st);

	/// @brief: Pointer to the font of this label
	const uv_font_st *font;
	/// @brief: Text color
	color_t color;
	color_t bg_color;
	/// @brief: Null-terminated string which will be shown on the screen
	char *str;
	/// @brief: Label alignment
	alignment_e align;
	///@brief: Image scale multiplier
	float scale;
} uv_uilabel_st;


#undef this
#define this ((uv_uilabel_st*)me)


void uv_uilabel_init(void *me, const uv_font_st *font,
		alignment_e alignment, color_t color, color_t bgcolor, char *str);

/// @brief: Step function which should be called every step cycle
void uv_uilabel_step(void *me, uv_touch_st *touch, uint16_t step_ms, const uv_bounding_box_st *pbb);

#define this		((uv_uilabel_st*)me)

/// @brief: sets the label scale
void uv_uilabel_set_scale(void *me, float scale);

/// @brief: Set's the text of all objects which are inherited from uv_uilabel_st
void uv_uilabel_set_text(void *me, char *str);


/// @brief: Sets the color of the label text
void uv_uilabel_set_color(void *me, color_t c);

/// @brief: Sets the background color
void uv_uilabel_set_bg_color(void *me, color_t c);


#undef this








#define this ((uv_uidigit_st*)me)



/// @brief: Typedef for digit label. Should be used with CONFIG_UI_NUM_XXXXX reduced fonts and
/// they can only contain numbers, comma and periods.
typedef struct {
	EXTENDS(uv_uilabel_st);

	unsigned int divider;
	char str[13];
	char format[10];
	int value;
} uv_uidigit_st;



/// @brief: Initializes the digit label.
///
/// @param format: Printf-format for showing the digit. Without fractions the
/// format should contain only one number. If fractions are used, it should define
/// exactly 2 numbers separated by a dot or a comma.
void uv_uidigit_init(void *me, const uv_font_st *font,
		alignment_e alignment, color_t color, color_t bgcolor, char *format, int value);


static inline void uv_uidigit_step(void *me, uv_touch_st *touch, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {
	uv_uilabel_step(me, touch, step_ms, pbb);
}

void uv_uidigit_set_value(void *me, int value);



/// @brief: Sets the color of the label text
static inline void uv_uidigit_set_color(void *me, color_t c) {
	uv_uilabel_set_color(me, c);
}

/// @brief: Sets the background color
static inline void uv_uidigit_set_bg_color(void *me, color_t c) {
	uv_uilabel_set_bg_color(me, c);
}

/// @brief: If set to true, uidigit will also show fractions divided by *value*
static inline void uv_uidigit_set_divider(void *me, unsigned int value) {
	if (!value) { value = 1; }
	this->divider = value;
	uv_ui_refresh(this);
}

static inline void uv_uidigit_set_scale(void *me, float scale) {
	uv_uilabel_set_scale(me, scale);
}


static inline int uv_uidigit_get_value(void *me) {
	return this->value;
}


/// @brief: Draws raw text on the screen.
/// Should be used only inside this hal library

void _uv_ui_draw_mtext(int16_t x, int16_t y, const uv_font_st *font,
		const alignment_e align, const color_t color, const color_t bgcolor,
		const char *str, const float scale, const uv_bounding_box_st *maskbb);

static inline void _uv_ui_draw_text(int16_t x, int16_t y, const uv_font_st *font,
		const alignment_e align, const color_t color, const color_t bgcolor,
		const char *str, const float scale) {
	uv_bounding_box_st bb = { 0, 0, LCD_W_PX, LCD_H_PX };
	_uv_ui_draw_mtext(x, y, font, align, color, bgcolor, str, scale, &bb);
}


/// @brief: Returns the strings length in pixels.
/// Takes new lines in account when calculating the length.
int16_t uv_ui_text_width_px(char *str, const uv_font_st *font, float scale);


/// @brief: Returns the string height in pixels
int16_t uv_ui_text_height_px(char *str, const uv_font_st *font, float scale);


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UILABEL_H_ */
