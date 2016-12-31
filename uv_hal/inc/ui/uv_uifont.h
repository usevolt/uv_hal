/*
 * uv_uifont.h
 *
 *  Created on: Oct 17, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UIFONT_H_
#define UV_HAL_INC_UI_UV_UIFONT_H_


#include <uv_hal_config.h>
#include <uv_utilities.h>

/* -------------------------------------------------------------------------------- */
/* -- �GUI FONTS                                                                 -- */
/* -- Source: http://www.mikrocontroller.net/user/show/benedikt                  -- */
/* -------------------------------------------------------------------------------- */
typedef struct
{
	unsigned char* p;
	int16_t char_width;
	int16_t char_height;
	int8_t index_offset;
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

#if CONFIG_UI_NUM_12X20
   extern const uv_font_st num_12X20;
#endif
#if CONFIG_UI_NUM_16X26
   extern const uv_font_st num_16X26;
#endif
#if CONFIG_UI_NUM_22X36
   extern const uv_font_st num_22X36;
#endif
#if CONFIG_UI_NUM_24X40
   extern const uv_font_st num_24X40;
#endif
#if CONFIG_UI_NUM_32X53
   extern const uv_font_st num_32X53;
#endif


#endif /* UV_HAL_INC_UI_UV_UIFONT_H_ */
