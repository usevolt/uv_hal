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

#ifndef UV_HAL_INC_UV_FT81X_H_
#define UV_HAL_INC_UV_FT81X_H_

#include "uv_utilities.h"
#include "uv_ui_common.h"

#if CONFIG_FT81X

#define FT810		1
#define FT811		2
#define FT812		3
#define FT813		4


#if !defined(CONFIG_FT81X_TYPE)
#error "CONFIG_FT81X_TYPE should be defined as FT810 ... FT813 depending which IC is controlled."
#endif
#if !defined(CONFIG_FT81X_SPI_CHANNEL)
#error "CONFIG_FT81X_SPI_CHANNEL should define the SPI channel used for FT81X."
#endif
#if !defined(CONFIG_FT81X_SSEL)
#error "CONFIG_FT81X_SSEL should define the slave ID for SPI channel"
#endif
#if !defined(CONFIG_FT81X_PD_IO)
#error "CONFIG_FT81X_PD_IO should define the I/O pin used for power down."
#endif
#if !CONFIG_FT81X_PCLK_HZ
#error "CONFIG_FT81X_PCLK_HZ should define the display clock cycle in Hz."
#endif
#if !CONFIG_FT81X_HCYCLE
#error "CONFIG_FT81X_HCYCLE should define the number of PCLK's the horizontal cycle takes at total.\
 Usually refered as Horizontal cycle in LCD datasheets."
#endif
#if !CONFIG_FT81X_HSIZE
#error "CONFIG_FT81X_HSIZE should define the number of pixels on a LCD horizontal line.\
 Usually referred as Horizontal display period on LDC datasheets."
#endif
#if !defined(CONFIG_FT81X_HSYNC1)
#error "CONFIG_FT81X_HSYNC1 should define the number of clock cycles the HSYNC signal\
 is down at the start of a horizontal line. Usually referred as horizontal pulse width."
#endif
#if !defined(CONFIG_FT81X_HSYNC0)
#error "CONFIG_FT81X_HSYNC0 should define how many clock cycles the HSYNC signal\
 is high before toggling. Usually not referred on LCD datasheets (== 0)."
#endif
#if !defined(CONFIG_FT81X_HOFFSET)
#error "CONFIG_FT81X_HOFFSET should define the number of clock cycles from HSYNC falling\
 until sending the pixel data. Usually referred as back porch. Note that this should be \
 greater than HSYNC1 since it is included in this value."
#endif
#if !CONFIG_FT81X_VCYCLE
#error "CONFIG_FT81X_VCYCLE should define the number of HSYNC's the vertical cycle takes at total.\
 Usually refered as Vertical cycle in LCD datasheets."
#endif
#if !CONFIG_FT81X_VSIZE
#error "CONFIG_FT81X_VSIZE should define the number of pixels on a LCD vertical line.\
 Usually referred as Vertical display period on LDC datasheets."
#endif
#if !defined(CONFIG_FT81X_VSYNC1)
#error "CONFIG_FT81X_VSYNC1 should define the number of clock cycles the VSYNC signal\
 is down at the start of a vertical line. Usually referred as vertical pulse width."
#endif
#if !defined(CONFIG_FT81X_VSYNC0)
#error "CONFIG_FT81X_VSYNC0 should define how many clock cycles the VSYNC signal\
 is high before toggling. Usually not referred on LCD datasheets (== 0)."
#endif
#if !defined(CONFIG_FT81X_VOFFSET)
#error "CONFIG_FT81X_VOFFSET should define the number of clock cycles from VSYNC falling\
 until sending the pixel data. Usually referred as back porch. Note that this should be \
 greater than VSYNC1 since it is included in this value."
#endif
#if !defined(CONFIG_FT81X_PCLK_POL)
#error "CONFIG_FT81X_PCLK_POL defines the PCLK polarity. Should be either FT81X_PCLK_POL_RISING or\
 FT81X_PCLK_POL_FALLING."
#endif
#if !defined(CONFIG_FT81X_CSPREAD)
#error "CONFIG_FT81X_CSPREAD should be defined as 0 for normal operation or 1 for reduced system noice operation."
#endif
#if !CONFIG_FT81X_BACKLIGHT_PWM_FREQ_HZ
#error "CONFIG_FT81X_BACKLIGHT_PWM_FREQ_HZ should define the backlight PWM frequency"
#elif (CONFIG_FT81X_BACKLIGHT_PWM_FREQ_HZ > 10000)
#error "CONFIG_FT81X_BACKLIGHT_PWM_FREQ_HZ maximum value is 10000."
#endif
#if !defined(CONFIG_FT81X_BACKLIGHT_INVERT)
#error "CONFIG_FT81X_BACKLIGHT_INVERT should be defined either 1 or 0 depending if backlight PWM duty cycle\
 should be inverted."
#endif
#if !defined(CONFIG_FT81X_SCREEN_COLOR)
#error "CONFIG_FT81X_SCREEN_COLOR should define the default screen color which is set\
 after swapping the DL buffer"
#endif
#if !defined(CONFIG_FT81X_MEDIA_MAXSIZE)
#error "CONFIG_FT81X_MEDIA_MAXSIZE should define the maximum size of a media file in bytes. Note that this\
 amount of memory is used for downloading the media files to the Media RAM, and effectively reduces the\
 available media RAM by the same amount."
#endif

#define FT81X_PCLK_POL_RISING	0
#define FT81X_PCLK_POL_FALLING	1









/// @brief: Bitmap formats for custom fonts
enum {
	BITMAP_FORMAT_L1 = 0,
	BITMAP_FORMAT_L4 = 2,
	BITMAP_FORMAT_L8 = 3,
	BITMAP_FORMAT_ARGB4 = 6,
	BITMAP_FORMAT_RGB565 = 7,
	BITMAP_FORMAT_PALETTED565 = 14,
	BITMAP_FORMAT_PALETTED4444 = 15,
	BITMAP_FORMAT_PALETTED8 = 16
};
typedef uint8_t bitmap_format_e;



/// @brief: FT81X font metric declaration
typedef struct {
	uint8_t char_widths[128];
	bitmap_format_e bitmap_format;
	uint32_t font_line_stride;
	uint32_t font_width;
	uint32_t font_height;
	void *data_ptr;
} uv_fontmetric_st;


/// @brief: The maximum size of a single graphic bitmap
#define FT81X_GRAPHIC_RAM_MAX_SIZE		(0x100000)
#define FT81X_PREPROCESSOR_SIZE			4096
/// @brief: The address of the MEDIAFIFO used for downloading the media files
#define FT81X_MEDIAFIFO_ADDR			(FT81X_GRAPHIC_RAM_MAX_SIZE - CONFIG_FT81X_MEDIA_MAXSIZE)


/// @brief: Extern declaration of the memory buffer for loading bitmaps.
/// This buffer can be used for other purposes.
extern volatile uint8_t ft81x_buffer[FT81X_PREPROCESSOR_SIZE];


#define FT81X_BACKLIGHT_MAX	100


#define FT81X_RAMDL_SIZE		0x2000

/// @brief: Returns the maximum display list RAM usage
uint32_t uv_ft81x_get_ramdl_usage(void);






#endif


#endif /* UV_HAL_INC_UV_FT81X_H_ */
