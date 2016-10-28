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
#include "uv_utilities.h"
#include "ui/uv_uifont.h"


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
} uv_uilabel_st;


/// @brief: Step function which should be called every step cycle
void uv_uilabel_step(void *me, uv_touch_st *touch, uint16_t step_ms);

#define this		((uv_uilabel_st*)me)


/// @brief: Set's the text of all objects which are inherited from uv_uilabel_st
static inline void uv_uilabel_set_text(void *me, char *str) {
	uv_ui_refresh(me);
	this->str = str;
}


/// @brief: Sets the color of the label text
static inline void uv_uilabel_set_color(void *me, color_t c) {
	uv_ui_refresh(me);
	this->color = c;
}

/// @brief: Sets the background color
static inline void uv_uilabel_set_bg_color(void *me, color_t c) {
	uv_ui_refresh(me);
	this->bg_color = c;
}


static inline void uv_uilabel_init(void *me, const uv_font_st *font,
		alignment_e alignment, color_t color, color_t bgcolor, char *str) {
	uv_uiobject_init((void*) this);
	this->font = font;
	this->str = str;
	this->align = alignment;
	this->color = color;
	this->bg_color = bgcolor;
}


/// @brief: Draws raw text on the screen.
/// Should be used only inside this hal library
void _uv_ui_draw_text(uint16_t x, uint16_t y, const uv_font_st *font,
		alignment_e align, color_t color, color_t bgcolor, char *str);


#undef this


#endif /* UV_HAL_INC_UI_UV_UILABEL_H_ */
