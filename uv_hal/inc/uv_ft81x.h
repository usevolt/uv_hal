/*
 * uv_ft81x.h
 *
 *  Created on: Jul 20, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UV_FT81X_H_
#define UV_HAL_INC_UV_FT81X_H_

#include "uv_utilities.h"
#include "uv_spi.h"

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
#endif
#if !defined(CONFIG_FT81X_BACKLIGHT_INVERT)
#error "CONFIG_FT81X_BACKLIGHT_INVERT should be defined either 1 or 0 depending if backlight PWM duty cycle\
 should be inverted."
#endif
#if !defined(CONFIG_FT81X_SCREEN_COLOR)
#error "CONFIG_FT81X_SCREEN_COLOR should define the default screen color which is set\
 after swapping the DL buffer"
#endif

#define FT81X_PCLK_POL_RISING	0
#define FT81X_PCLK_POL_FALLING	1


typedef enum {
	FONT_1 = 16,
	FONT_2,
	FONT_3,
	FONT_4,
	FONT_5,
	FONT_6,
	FONT_7,
	// FONT_8, 9 & 10 disabled, replaced with FONT_17, 18, 19 instead
	//
	FONT_11,
	FONT_12,
	FONT_13,
	FONT_14,
	FONT_15,
	FONT_16,
	// note: The biggest fonts are not set to any bitmap handles,
	// use anti-alised font handles instead
	FONT_17 = FONT_7 + 1,
	FONT_18 = FONT_7 + 2,
	FONT_19 = FONT_7 + 3
} ft81x_fonts_e;

/// @brief: Wrapper for font data for UI library
typedef struct {
	uint16_t char_height;
	uint8_t index;
} ft81x_font_st;
#define FONT_COUNT						(FONT_16 - FONT_1 + 1)
extern ft81x_font_st ft81x_fonts[FONT_COUNT];


typedef enum {
	FT81X_ALIGN_LEFT_TOP = 0,
	FT81X_ALIGN_LEFT_CENTER = 0x400,
	FT81X_ALIGN_RIGHT_TOP = 0x800,
	FT81X_ALIGN_RIGHT_CENTER = 0x800 | 0x400,
	FT81X_ALIGN_CENTER = 0x600,
	FT81X_ALIGN_CENTER_TOP = 0x200
} ft81x_align_e;



/// @brief: Width of the LCD in pixels
#define LCD_W_PX		CONFIG_FT81X_HSIZE

/// @brief: Height of the LCD in pixels
#define LCD_H_PX		CONFIG_FT81X_VSIZE

/// @brief: Converts from relative 0.0f - 1.0f width to actual pixel width
#define LCD_W(rel_w)	(LCD_W_PX * (rel_w))
#define LCD_WPPT(w_ppt) (LCD_W_PX * w_ppt / 1000)

/// @brief: Converts from relative 0.0f - 1.0f height to actual pixel height
#define LCD_H(rel_h)	(LCD_H_PX * (rel_h))
#define LCD_HPPT(h_ppt)	(LCD_H_PX * h_ppt / 1000)



/// @brief: Color typedef
typedef struct {
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t a;
} color_st;
typedef uint32_t color_t;
// Macro which should be used when colors are assigned. This is to keep
// UI libraries compatible with different color spaces than ARGB8888.
#define C(x)		(x)


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


/// @brief: Bitmap formats for custom fonts
typedef enum {
	BITMAP_FORMAT_L1 = 0,
	BITMAP_FORMAT_L4 = 2,
	BITMAP_FORMAT_L8 = 3
} _bitmap_format_e;
typedef _bitmap_format_e bitmap_format_e;



/// @brief: FT81X font metric declaration
typedef struct {
	uint8_t char_widths[128];
	bitmap_format_e bitmap_format;
	uint32_t font_line_stride;
	uint32_t font_width;
	uint32_t font_height;
	void *data_ptr;
} uv_fontmetric_st;



/// @brief: Initializes the FT81X LCD driver module. Will be called from
/// the HAL task
void uv_ft81x_init(void);


/// @brief: Swaps the display list buffers and makes all latest UI modifications visible
/// on the LCD display.
void uv_ft81x_dlswap(void);


/// @brief: Sets the backlight brightness
///
/// @param percent: The percen tof the backlight. 0 = minimum, 100 = maximum
void uv_ft81x_set_backlight(uint8_t percent);


/// brief: Returns the current backlight brightness
uint8_t uv_ft81x_get_backlight(void);


/// @brief: Clears the whole screen to color **c**
void uv_ft81x_clear(color_t c);


/// @brief: Returns the maximum display list RAM usage
uint32_t uv_ft81x_get_ramdl_usage(void);


/// @brief: Draws a filled circle to the screen
///
/// @param x: The X coordinate of the point's center
/// @param y: The Y coordinate of the point's center
/// @param color: The color of the point
/// @param diameter: The point diameter in pixels
void uv_ft81x_draw_point(int16_t x, int16_t y, color_t color, uint16_t diameter);


/// @brief: Draws a filled rounded rectangle on the screen
///
/// @param x: The X coordinate of the upper left corner
/// @param y: The Y coordinate of the upper left corner
/// @param wdith: Width of the rectangle in pixels
/// @param height: Height of the rectangle in pixels
/// @param radius: The rounding radius in the corners
/// @param color: The fill color
void uv_ft81x_draw_rrect(const int16_t x, const int16_t y,
		const uint16_t width, const uint16_t height,
		const uint16_t radius, const color_t color);


/// @brief: helper function to draw a shadowed rounded rectangle
void uv_ft81x_draw_shadowrrect(const int16_t x, const int16_t y,
		const uint16_t width, const uint16_t height,
		const uint16_t radius, const color_t color,
		const color_t highlight_c, const color_t shadow_c);


/// @brief: Draws a single line on the screen
void uv_ft81x_draw_line(const int16_t start_x, const int16_t start_y,
		const int16_t end_x, const int16_t end_y,
		const uint16_t width, const color_t color);


/// @brief: Trigger the touch screen calibration sequence
///
/// @param transform_matrix: If given, function will write the transform matrix information
/// to the address pointed by this parameter. Note that 6 words (6*4 bytes) should be
/// reserved for the transform matrix.
void uv_ft81x_touchscreen_calibrate(uint32_t *transform_matrix);


/// @brief: Sets the transform matrix data. This should be called by the user application
/// if such transform matrix data already exists in the non-volatile memory
void uv_ft81x_touchscreen_set_transform_matrix(uint32_t *transform_matrix);


/// @brief: Gets the touch from the FT81X touchpanel
///
/// @return: True if the screen is touched, false if it is not touched
///
/// @param x: Pointer to where the x px coordinate will be written (or NULL)
/// @param y: Pointer to where the y px coordinate will be written (or NULL)
bool uv_ft81x_get_touch(int16_t *x, int16_t *y);


/// @brief: Draws a letter on the screen
///
/// @param c: The letter to be drawn
/// @param font: The FT81X font index (16-33)
/// @param x: The top-left X coordinate
/// @param y: The top-left Y coordinate
/// @param color: The drawing color
void uv_ft81x_draw_char(const char c, const uint16_t font, int16_t x, int16_t y, color_t color);


/// @brief: uses the FT81X co-processor to draw a text string to the display.
void uv_ft81x_draw_string(char *str, const ft81x_fonts_e font,
		int16_t x, int16_t y, ft81x_align_e align, color_t color);


/// @brief: Returns the font height in pixels
uint8_t uv_ft81x_get_font_height(const ft81x_fonts_e font);


/// @brief: Sets the drawing mask which masks all drawing functions to the masked area
void uv_ft81x_set_mask(int16_t x, int16_t y, uint16_t width, uint16_t height);

#endif


#endif /* UV_HAL_INC_UV_FT81X_H_ */
