/*
 * uv_lcd.h
 *
 *  Created on: Aug 18, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UV_LCD_H_
#define UV_HAL_INC_UV_LCD_H_

#include "uv_hal_config.h"

#if CONFIG_LCD

#include "uv_utilities.h"
#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
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


/// @brief: Values used for CONFIG_LCD_BITS_PER_PIXEL configuration define
#define LCD_1_BPP			0
#define LCD_2_BPP			1
#define LCD_4_BPP			2
#define LCD_8_BPP			3
#define LCD_16_BPP			4
#define LCD_24_BPP			5
#define LCD_16_BPP_RB565	6
#define LCD_12_BPP_RGB444	7

/// @brief: Defines the PIXEL data type. Depends on how many bits are used per pixel
#if CONFIG_LCD_BITS_PER_PIXEL == LCD_24_BPP
#define LCD_PIXEL_TYPE	uint32_t
#elif CONFIG_LCD_BITS_PER_PIXEL == LCD_16_BPP
#define LCD_PIXEL_TYPE	uint16_t
#elif CONFIG_LCD_BITS_PER_PIXEL == LCD_8_BPP
#define LCD_PIXEL_TYPE	uint8_t
#else
#error "Unimplemented bits per pixel setting."
#endif

/// @brief: Width of the LCD in pixels
#define LCD_W_PX		CONFIG_LCD_PIXELS_PER_LINE

/// @brief: Height of the LCD in pixels
#define LCD_H_PX		CONFIG_LCD_LINES_PER_PANEL

/// @brief: Converts from relative 0.0f - 1.0f width to actual pixel width
#define LCD_W(rel_w)	(LCD_W_PX * rel_w)

/// @brief: Converts from relative 0.0f - 1.0f height to actual pixel height
#define LCD_H(rel_h)	(LCD_H_PX * rel_h)

#if (CONFIG_LCD_BITS_PER_PIXEL != LCD_24_BPP)
#error "Incorrect bits per pixel setting. Only RGB888 (24-bits) format is implemented"
#endif

/// @brief: Color type. All colors are given in RGB888.
typedef int32_t color_t;
enum {
	COLOR_BLACK = 0x000000,
	COLOR_WHITE = 0xFFFFFF,
	COLOR_BLUE = 0x0000FF,
	COLOR_RED = 0xFF0000,
	COLOR_GREEN = 0x00FF00
};

/// @brief: Initializes the LCD module
uv_errors_e uv_lcd_tft_init(void);

/// @brief: Sets the specific pixels from the LCD. Note that for performance reasons
/// the function doesnt check for overindexing.
/// @pre: *x* and *y* should be smaller than the LCD maximum size.
void uv_lcd_draw_pixel(uint32_t x, uint32_t y, color_t color);

/// @brief: Draws a solid color rectangle on the screen
///
/// @param x: The X coordinate of the left-top corner of the rectangle
/// @param y: The Y coordinate of the left-top corner of the rectangle
/// @param width: The width of the rectangle in pixels
/// @param height: The height of the rectangle in pixels
/// @param color: The color of the rectangle
void uv_lcd_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t color);

/// @brief: Draws a solid color frame on the screen
///
/// @param x: The X coordinate of the left-top corner of the frame
/// @param y: The Y coordinate of the left-top corner of the frame
/// @param width: The width of the frame in pixels
/// @param height: The height of the frame in pixels
/// @param border: The thickness of the frame
/// @param color: The color of the frame
void uv_lcd_draw_frame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t border, color_t color);


/// @brief: Draws a vertical gradient rectangle to the display
///
/// @param x: The X coordinate of the left-top corner of the rectangle
/// @param y: The Y coordinate of the left-top corner of the rectangle
/// @param width: The width of the rectangle in pixels
/// @param height: The height of the rectangle in pixels
/// @param t_color: Top color
/// @param b_color: Bottom color
void uv_lcd_draw_v_gradient(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t t_color, color_t b_color);


#endif

#endif /* UV_HAL_INC_UV_LCD_H_ */
