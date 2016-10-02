/*
 * uv_label.h
 *
 *  Created on: Sep 30, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_LABEL_H_
#define UV_HAL_INC_UI_UV_LABEL_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "ui/uv_object.h"


/* -------------------------------------------------------------------------------- */
/* -- �GUI FONTS                                                                 -- */
/* -- Source: http://www.mikrocontroller.net/user/show/benedikt                  -- */
/* -------------------------------------------------------------------------------- */
typedef struct
{
   unsigned char* p;
   int16_t char_width;
   int16_t char_height;
} uv_font_st;


#define Auml		"\x8e"
#define auml		"\x84"
#define Ouml		"\x99"
#define ouml		"\x94"



#if CONFIG_UI_FONT_4X6
   extern const uv_font_st font_4X6;
#endif
#if CONFIG_UI_FONT_5X8
   extern const uv_font_st font_5X8;
#endif
#if CONFIG_UI_FONT_5X12
   extern const uv_font_st font_5X12;
#endif
#if CONFIG_UI_FONT_6X8
   extern const uv_font_st font_6X8;
#endif
#if CONFIG_UI_FONT_6X10
   extern const uv_font_st font_6X10;
#endif
#if CONFIG_UI_FONT_7X12
   extern const uv_font_st font_7X12;
#endif
#if CONFIG_UI_FONT_8X8
   extern const uv_font_st font_8X8;
#endif
#if CONFIG_UI_FONT_8X12
   extern const uv_font_st font_8X12;
#endif
#if CONFIG_UI_FONT_8X14
   extern const uv_font_st font_8X14;
#endif
#if CONFIG_UI_FONT_10X16
   extern const uv_font_st font_10X16;
#endif
#if CONFIG_UI_FONT_12X16
   extern const uv_font_st font_12X16;
#endif
#if CONFIG_UI_FONT_12X20
   extern const uv_font_st font_12X20;
#endif
#if CONFIG_UI_FONT_16X26
   extern const uv_font_st font_16X26;
#endif
#if CONFIG_UI_FONT_22X36
   extern const uv_font_st font_22X36;
#endif
#if CONFIG_UI_FONT_24X40
   extern const uv_font_st font_24X40;
#endif
#if CONFIG_UI_FONT_32X53
   extern const uv_font_st font_32X53;
#endif

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
	EXTENDS(uv_ui_object_st);

	/// @brief: Pointer to the font of this label
	const uv_font_st *font;
	/// @brief: Text color
	color_t color;
	/// @brief: Null-terminated string which will be shown on the screen
	char *str;
	/// @brief: Lable alignment
	alignment_e align;
} uv_label_st;


/// @brief: Step function which should be called every step cycle
void uv_label_step(void *me, uint16_t step_ms);

#define this		((uv_label_st*)me)


/// @brief: Set's the text of all objects which are inherited from uv_label_st
static inline void uv_label_set_text(void *me, char *str) {
	uv_ui_refresh(me);
	this->str = str;
}


/// @brief: Sets the color of the label text
static inline void uv_label_set_color(void *me, color_t c) {
	uv_ui_refresh(me);
	this->color = c;
}


static inline void uv_label_init(void *me, const uv_font_st *font,
		alignment_e alignment, color_t color, char *str) {
	this->font = font;
	this->str = str;
	this->align = alignment;
	this->color = color;
	uv_ui_object_init((void*) this, NULL, 0, 0, 0, 0, false, uv_label_step);
}

#undef this


#endif /* UV_HAL_INC_UI_UV_LABEL_H_ */
