/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UV_HAL_INC_UV_LCD_H_
#define UV_HAL_INC_UV_LCD_H_

#include "uv_hal_config.h"

#if CONFIG_LCD

#include "uv_utilities.h"
#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif

#if !defined(CONFIG_LCD_RGB_TO_BGR)
#error "CONFIG_LCD_RGB_TO_BGR should be defined as 1 or 0 depending if the red and blue color space should be swapped"
#endif
#if !defined(CONFIG_LCD_FREQ)
#error "CONFIG_LCD_FREQ should define the LCD clock frequency in Hz"
#endif
#if !defined(CONFIG_LCD_BUFFER_ADDRESS)
#error "CONFIG_LCD_BUFFER_ADDRESS should define the frame buffer RAM address. Usually external RAM is required."
#endif
#if !defined(CONFIG_LCD_PIXELS_PER_LINE)
#error "CONFIG_LCD_PIXELS_PER_LINE should define the LCD width in pixels"
#endif
#if !defined(CONFIG_LCD_HSYNC_PULSE_WIDTH)
#error "CONFIG_LCD_HSYNC_PULSE_WIDTH should define the horizontal synchronization pulse width in LCD CLK.\
 Refer to the LCD datasheet for correct value."
#endif
#if !defined(CONFIG_LCD_H_FRONT_PORCH)
#error "CONFIG_LCD_H_FRONT_PORCH should define the horizontal front porch in LCD CLK.\
 Refer to the LCD datasheet for correct value."
#endif
#if !defined(CONFIG_LCD_H_BACK_PORCH)
#error "CONFIG_LCD_H_BACK_PORCH should define the horizontal back porch in LCD CLK.\
 Refer to the LCD datasheet for correct value."
#endif
#if !defined(CONFIG_LCD_LINES_PER_PANEL)
#error "CONFIG_LCD_LINES_PER_PANEL should define the LCD height in pixels/lines.\
 Refer to the LCD datasheet for correct value."
#endif
#if !defined(CONFIG_LCD_VSYNC_PULSE_WIDTH)
#error "CONFIG_LCD_VSYNC_PULSE_WIDTH should define the vertical synchronization pulse width in lines (Th).\
 Refer to the LCD datasheet for correct value."
#endif
#if !defined(CONFIG_LCD_V_FRONT_PORCH)
#error "CONFIG_LCD_V_FRONT_PORCH should define the vertical front porch in lines (Th).\
 Refer to the LCD datasheet for correct value."
#endif
#if !defined(CONFIG_LCD_V_BACK_PORCH)
#error "CONFIG_LCD_BACK_PORCH should define the vertical back porch in lines (Th).\
 Refer to the LCD datasheet for correct value."
#endif
#if !defined(CONFIG_LCD_INVERT_PANEL_CLOCK)
#error "CONFIG_LCD_INVERT_PANEL_CLOCK 1: Data is updated on falling edge of the LCD CLK. \
0: Data is updated on rising edge of the LCD CLK. If the LCD reads data on rising edge, set this to falling edge."
#endif
#if !defined(CONFIG_LCD_INVERT_HSYNC)
#error "CONFIG_LCD_INVERT_HSYNC inverts the HSYNC logic. 0: HSYNC is active high, 1: HSYNC is active low."
#endif
#if !defined(CONFIG_LCD_INVERT_VSYNC)
#error "CONFIG_LCD_INVERT_VSYNC inverts the VSYNC logic. 0: VSYNC is active high, 1: VSYNC is active low."
#endif
#if !defined(CONFIG_LCD_BITS_PER_PIXEL)
#error "CONFIG_LCD_BITS_PER_PIXEL defines the color resolution of the display. Refer to uv_lcd.h for correct values."
#endif
#if !defined(CONFIG_LCD_PWR_IOCON)
#error "CONFIG_LCD_PWR_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_DCLK_IOCON)
#error "CONFIG_LCD_DCLK_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_ENAB_M_IOCON)
#error "CONFIG_LCD_ENAB_M_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VSYNC_IOCON)
#error "CONFIG_LCD_VSYNC_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_HSYNC_IOCON)
#error "CONFIG_LCD_HSYNC_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_LE_IOCON)
#error "CONFIG_LCD_LE_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_CLKIN_IOCON)
#error "CONFIG_LCD_CLKIN_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD0_IOCON)
#error "CONFIG_LCD_VD0_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD1_IOCON)
#error "CONFIG_LCD_VD1_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD2_IOCON)
#error "CONFIG_LCD_VD2_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD3_IOCON)
#error "CONFIG_LCD_VD3_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD4_IOCON)
#error "CONFIG_LCD_VD4_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD5_IOCON)
#error "CONFIG_LCD_VD5_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD6_IOCON)
#error "CONFIG_LCD_VD6_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD7_IOCON)
#error "CONFIG_LCD_VD7_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD8_IOCON)
#error "CONFIG_LCD_VD8_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD9_IOCON)
#error "CONFIG_LCD_VD9_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD10_IOCON)
#error "CONFIG_LCD_VD10_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD11_IOCON)
#error "CONFIG_LCD_VD11_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD12_IOCON)
#error "CONFIG_LCD_VD12_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD13_IOCON)
#error "CONFIG_LCD_VD13_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD14_IOCON)
#error "CONFIG_LCD_VD14_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD15_IOCON)
#error "CONFIG_LCD_VD15_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD16_IOCON)
#error "CONFIG_LCD_VD16_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD17_IOCON)
#error "CONFIG_LCD_VD17_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD18_IOCON)
#error "CONFIG_LCD_VD18_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD19_IOCON)
#error "CONFIG_LCD_VD19_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD20_IOCON)
#error "CONFIG_LCD_VD20_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD21_IOCON)
#error "CONFIG_LCD_VD21_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD22_IOCON)
#error "CONFIG_LCD_VD22_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_VD23_IOCON)
#error "CONFIG_LCD_VD23_IOCON should define the IO configuration settings for this LCD pin."
#endif
#if !defined(CONFIG_LCD_DOUBLE_BUFFER)
#error "CONFIG_LCD_DOUBLE_BUFFER should be defined as 1 or 0, depending if double buffering\
 should be enabled"
#endif
#if CONFIG_LCD_DOUBLE_BUFFER
#if !defined(CONFIG_LCD_DOUBLE_BUFFER_ADDRESS)
#error "CONFIG_LCD_DOUBLE_BUFFER_ADDRESS should define the 2nd frame buffer RAM address. \
Usually external RAM is required"
#endif
#endif
#if CONFIG_LCD_TOUCHSCREEN
#if !defined(CONFIG_LCD_X_L_ADC) || !defined(CONFIG_LCD_X_L_GPIO)
#error "CONFIG_LCD_X_L_ADC should define the ADC channel used to X Left input and\
 CONFIG_LCD_X_L_GPIO should define the same GPIO pin."
#endif
#if !defined(CONFIG_LCD_X_R_ADC) || !defined(CONFIG_LCD_X_R_GPIO)
#error "CONFIG_LCD_X_R_ADC should define the ADC channel used for X Right input and\
 CONFIG_LCD_X_R_GPIO should define the same GPIO pin."
#endif
#if !defined(CONFIG_LCD_Y_T_ADC) || !defined(CONFIG_LCD_Y_T_GPIO)
#error "CONFIG_LCD_Y_T_ADC should define the ADC channel used for Y Top input and\
 CONFIG_LCD_Y_T_GPIO should define the same GPIO pin."
#endif
#if !defined(CONFIG_LCD_Y_B_ADC) || !defined(CONFIG_LCD_Y_B_GPIO)
#error "CONFIG_LCD_Y_B_ADC should define the ADC channel used for Y Bottom input and\
 CONFIG_LCD_Y_B_GPIO should define the same GPIO pin."
#endif
#if !defined(CONFIG_LCD_TOUCH_THRESHOLD)
#error "CONFIG_LCD_TOUCH_THRESHOLD should defined the threshold value which is used to detect\
 if the screen is touched or not. ADC converter value smaller than this is recognized as a touch."
#endif
#if !defined(CONFIG_LCD_X_MAX)
#error "CONFIG_LCD_X_MAX should define the touchscreen ADC maximum value for X axis"
#endif
#if !defined(CONFIG_LCD_Y_MAX)
#error "CONFIG_LCD_X_MAX should define the touchscreen ADC maximum value for Y axis"
#endif
#if !defined(CONFIG_LCD_X_MIN)
#error "CONFIG_LCD_X_MIN should define the touchscreen ADC minimum value for X axis"
#endif
#if !defined(CONFIG_LCD_Y_MIN)
#error "CONFIG_LCD_X_MIN should define the touchscreen ADC minimum value for Y axis"
#endif
#endif

/// @brief: Values used for CONFIG_LCD_BITS_PER_PIXEL configuration define
#define LCD_1_BPP			0
#define LCD_2_BPP			1
#define LCD_4_BPP			2
#define LCD_8_BPP			3
#define LCD_16_BPP			4
#define LCD_24_BPP			5
#define LCD_16_BPP_RGB565	6
#define LCD_12_BPP_RGB444	7

/// @brief: Defines the PIXEL data type. Depends on how many bits are used per pixel
#if CONFIG_LCD_BITS_PER_PIXEL == LCD_24_BPP
#define LCD_PIXEL_TYPE	uint32_t
#elif CONFIG_LCD_BITS_PER_PIXEL == LCD_16_BPP
#define LCD_PIXEL_TYPE	uint16_t
#elif CONFIG_LCD_BITS_PER_PIXEL == LCD_8_BPP
#define LCD_PIXEL_TYPE	uint8_t
#elif CONFIG_LCD_BITS_PER_PIXEL == LCD_16_BPP_RGB565
#define LCD_PIXEL_TYPE	uint16_t
#else
#error "Unimplemented bits per pixel setting."
#endif

/// @brief: Width of the LCD in pixels
#define LCD_W_PX		CONFIG_LCD_PIXELS_PER_LINE

/// @brief: Height of the LCD in pixels
#define LCD_H_PX		CONFIG_LCD_LINES_PER_PANEL

/// @brief: Converts from relative 0.0f - 1.0f width to actual pixel width
#define LCD_W(rel_w)	(LCD_W_PX * (rel_w))

/// @brief: Converts from relative 0.0f - 1.0f height to actual pixel height
#define LCD_H(rel_h)	(LCD_H_PX * (rel_h))


#if (CONFIG_LCD_BITS_PER_PIXEL != LCD_24_BPP) && \
	(CONFIG_LCD_BITS_PER_PIXEL != LCD_16_BPP_RGB565)
#error "Incorrect bits per pixel setting. Only RGB888 (24-bits) format is implemented"
#endif


/// @brief: Converts RGB888 color to used color space.
/// For compatibility all colors should be defined with this macro.
#if CONFIG_LCD_BITS_PER_PIXEL == LCD_24_BPP
#define C(x)	(x)
#elif CONFIG_LCD_BITS_PER_PIXEL == LCD_16_BPP_RGB565
#define C(x)	((x & 0xFF000000) | \
				(((x & 0xFF0000) >> (16 + 3)) << 11) | \
				(((x & 0xFF00) >> (8 + 2)) << 5) | \
				((x & 0xFF) >> 3))
#else
#error "Converting from RGB888 to selected color space not implemented"
#endif

/// @brief: Color type. All colors are given in RGB888.
///
/// @note: Alpha channel is inverted: 0 means fully visible,
/// 0xFF means fully transparent
typedef struct {
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t a;
} rgb_st;
typedef int32_t color_t;

/// brief: Converts color_t to rgb_st
static inline rgb_st uv_ctorgb(color_t c) {
	return *((rgb_st*) &c);
}

/// @brief: Converts rgb_st to color_t
static inline color_t uv_rgbtoc(rgb_st rgb) {
	return *((color_t*) &rgb);
}


/// @brief: Struct for individual object's bounding box.
typedef struct {
	/// @brief: local left-most GLOBAL x coordinate relative to the parent
	int16_t x;
	/// @brief: Local top-most y-coordinate relative to the parent
	int16_t y;
	/// @brief: Width, growing to right
	int16_t width;
	/// @brief: Height, growing to bottom
	int16_t height;
} uv_bounding_box_st;


/// @brief: Helper macro for easier calls to LCD drawing functions as well as
/// GUI functions. Calculates x so that the created component will be
/// created horizontally center aligned around (x,y) point.
///
/// @example:
///	// will draw rectangle from (-50, 50) to (150,200)
///	uv_lcd_draw_rect(UI_ALIGN_H_CENTER(50, 50, 200, 200));
#define UI_ALIGN_H_CENTER(x, y, width, height)	(x) - ((width)/2), (y), (width), (height)

#define UI_ALIGN_V_CENTER(x, y, width, height)	(x), (y) - ((height) / 2), (width), (height)

#define UI_ALIGN_CENTER(x, y, width, height)	(x) - ((width)/2), (y) - ((height) / 2), (width), (height)

/// @brief: Desaturates the color
///
/// @param mult: 0.0f returns the same color as c, 1.0f returns fully desaturated value.
//color_t uv_cdesat(color_t c, float mult);

/// @brief: pointer to the display. Pixels are oriented as [y][x] two dimensional array
typedef LCD_PIXEL_TYPE lcd_line_t[CONFIG_LCD_PIXELS_PER_LINE];
extern lcd_line_t *lcd;


/// @brief: Initializes the LCD module
uv_errors_e _uv_lcd_init(void);


/// @brief: Sets the specific pixels from the LCD. Note that for performance reasons
/// the function doesnt check for overindexing.
/// @pre: *x* and *y* should be smaller than the LCD maximum size.
static inline void uv_lcd_draw_pixel(int32_t x, int32_t y, color_t color) {
	if (x < 0 || y < 0 || x > LCD_W_PX || y > LCD_H_PX) return;
	lcd[y][x] = color;
}

/// @brief: Draws a solid color rectangle as in *uv_lcd_draw_rect* for all pixels which
/// are inside the defined mask rectangle.
///
/// @param mask_x: The X coordinate of the left-top corner of the mask rectangle
/// @param mask_y: The Y coordinate of the left-top corner of the mask rectangle
/// @param mask_w: The width of the mask rectangle in pixels
/// @param mask_h: The height of the mask rectangle in pixels
void uv_lcd_draw_mrect(int32_t x, int32_t y, int32_t width, int32_t height, const color_t c,
		const uv_bounding_box_st *maskbb);

/// @brief: Draws a solid color rectangle on the screen
///
/// @param x: The X coordinate of the left-top corner of the rectangle
/// @param y: The Y coordinate of the left-top corner of the rectangle
/// @param width: The width of the rectangle in pixels
/// @param height: The height of the rectangle in pixels
/// @param color: The color of the rectangle
static inline void uv_lcd_draw_rect(int32_t x, int32_t y, int32_t width, int32_t height, const color_t color) {
	uv_bounding_box_st bb = { 0, 0, LCD_W_PX, LCD_H_PX };
	uv_lcd_draw_mrect(x, y, width, height, color, &bb);
}


/// @brief: Draws a solid color frame as in *uv_lcd_draw_frame* for all pixels which are inside
/// the defined mask rectangle.
///
/// @param mask_x: The X coordinate of the left-top corner of the mask rectangle
/// @param mask_y: The Y coordinate of the left-top corner of the mask rectangle
/// @param mask_w: The width of the mask rectangle in pixels
/// @param mask_h: The height of the mask rectangle in pixels
void uv_lcd_draw_mframe(int32_t x, int32_t y, int32_t width, int32_t height, const int32_t border,
		const color_t color, const uv_bounding_box_st *maskbb);

/// @brief: Draws a solid color frame on the screen
///
/// @param x: The X coordinate of the left-top corner of the frame
/// @param y: The Y coordinate of the left-top corner of the frame
/// @param width: The width of the frame in pixels
/// @param height: The height of the frame in pixels
/// @param border: The thickness of the frame
/// @param color: The color of the frame
static inline void uv_lcd_draw_frame(int32_t x, int32_t y, uint32_t width,
		uint32_t height, int32_t border, const color_t color) {
	uv_bounding_box_st bb = { 0, 0, LCD_W_PX, LCD_H_PX };
	uv_lcd_draw_mframe(x, y, width, height, border, color, &bb);
}

/// @brief: Requests the LCD module to swap double buffer. This should be called after the
/// display drawing is completed (if double buffering is enabled)
void uv_lcd_double_buffer_swap(void);


#if CONFIG_LCD_TOUCHSCREEN

/// @brief: Calibrates the touchscreen. The calibration should be called twice, each with
/// different positions, i.e. top left corner and bottom right corner.
/// The calibration algorithm takes the current pressing point and
/// sets it to correspond the pixel position given
void uv_lcd_touch_calib(uint16_t x, uint16_t y);

/// @brief: Clears all calibrations. For proper calibration sequence this should be called first
/// and after that uv_lcd_touch_calib should be called twice when the user presses the touchscreen.
void uv_lcd_touch_calib_clear(void);


/// @brief: Returns true if the user touches the touchscreen. Also if the touchscreen has been
/// correctly configured, returns the position of the touch in pixels.
bool uv_lcd_touch_get(int16_t *x, int16_t *y);

#endif


#endif

#endif /* UV_HAL_INC_UV_LCD_H_ */
