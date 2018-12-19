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

#if CONFIG_UI

#include "uv_ft81x.h"

typedef ft81x_font_st uv_font_st;

// non-antialiazed fonts not supported
//#define font4	ft81x_fonts[3]
//#define font5	ft81x_fonts[4]
//#define font6	ft81x_fonts[5]
//#define font7	ft81x_fonts[6]
//#define font8	ft81x_fonts[7]
//#define font9	ft81x_fonts[8]
//#define font10	ft81x_fonts[9]
#define font16	ft81x_fonts[0]
#define font20	ft81x_fonts[1]
#define font25	ft81x_fonts[2]
#define font28	ft81x_fonts[3]
#define font36	ft81x_fonts[4]
#define font49	ft81x_fonts[5]
#define font63	ft81x_fonts[6]
#define font83	ft81x_fonts[7]
#define font108	ft81x_fonts[8]


#endif

#endif /* UV_HAL_INC_UI_UV_UIFONT_H_ */


