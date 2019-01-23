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

#ifndef UV_HAL_INC_UV_FT81X_H_
#define UV_HAL_INC_UV_FT81X_H_

#include "uv_utilities.h"
#include "uv_spi.h"
#include "uv_w25q128.h"
#include "ui/uv_uimedia.h"

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




/// @brief: Wrapper for font data for UI library
typedef struct {
	uint16_t char_height;
	// defines the font index. Index 26-34 are anti-aliased fonts which are supported by this library
	uint8_t index;
	// handle defines the bitmap handle to be used for this font
	uint8_t handle;
} ft81x_font_st;


/// @brief: FT81x library supports only anti-aliased fonts with index 26 - 34
#define FT81X_MAX_FONT_COUNT				9
extern ft81x_font_st ft81x_fonts[FT81X_MAX_FONT_COUNT];



typedef enum {
	FT81X_VALIGN_TOP = 0,
	FT81X_VALIGN_CENTER = 0x400
} ft81x_valign_e;
#define FT81X_VALIGN_MASK 		0x400

typedef enum {
	FT81X_HALIGN_LEFT = 0,
	FT81X_HALIGN_CENTER = 0x200,
	FT81X_HALIGN_RIGHT = 0x800
} ft81x_halign_e;
#define FT81X_HALIGN_MASK 		(0x200 | 0x800)


typedef enum {
	FT81X_ALIGN_LEFT_TOP = FT81X_HALIGN_LEFT | FT81X_VALIGN_TOP,
	FT81X_ALIGN_LEFT_CENTER = FT81X_HALIGN_LEFT | FT81X_VALIGN_CENTER,
	FT81X_ALIGN_RIGHT_TOP = FT81X_HALIGN_RIGHT | FT81X_VALIGN_TOP,
	FT81X_ALIGN_RIGHT_CENTER = FT81X_HALIGN_RIGHT | FT81X_VALIGN_CENTER,
	FT81X_ALIGN_CENTER = FT81X_HALIGN_CENTER | FT81X_VALIGN_CENTER,
	FT81X_ALIGN_CENTER_TOP = FT81X_HALIGN_CENTER | FT81X_VALIGN_TOP
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

/// @brief: Returns a color which is brightened by removing *value* amount of 8-bit color
/// from all R, G and B color channels
color_t uv_uic_brighten(color_t c, int8_t value);

/// @brief: Copies the alpha channel from *dest* color to *src* color
static inline void uv_uic_copy_alpha(color_t *dest, color_t *src) {
	((color_st*) dest)->a = ((color_st *)src)->a;
}

/// @brief: Struct for individual object's bounding box.
typedef struct {
	/// @brief: local left-most x coordinate relative to the parent
	int16_t x;
	/// @brief: Local top-most y-coordinate relative to the parent
	int16_t y;
	/// @brief: Width, growing to right
	int16_t width;
	/// @brief: Height, growing to bottom
	int16_t height;
} uv_bounding_box_st;


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



/// @brief: Initializes the FT81X LCD driver module. Will be called from
/// the HAL task
///
/// @return: True if touchscreen calibration was requested,
/// otherwise false.
bool uv_ft81x_init(void);


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


#define FT81X_RAMDL_SIZE		0x2000

/// @brief: Returns the maximum display list RAM usage
uint32_t uv_ft81x_get_ramdl_usage(void);


/// @brief: Loads and decompresses a jpg image to the media RAM of FT81x from external memory module.
///
/// @return: The number of bytes that the image took from the memory. Since
/// the image is decompressed, the returned value is larger than the downloaded value.
/// In case of error, 0 is returned.
///
/// @note: Supported images which can be downloaded are baseline jpgs and 8-bit depth pngs with alpha channel.
/// To export usable images out from GIMP, disable all checkboxes in the export-dialog for the specific
/// file format. Baseline JPGs can be exported by unchecking the "Progressive" checkbox.
/// Gimp exports PNGs in PNG32-mode, which has to be converted to PNG8 with RGBA4444 color format.
/// The best tool for this is *pngquant*, which is used like this:
/// ´´´´pngquant 256 -f -o output.png input.png´´´´
/// Where 256 is the number of colors used. It can be smaller to reduce the file size, but it should be
/// in power of 2, i.e. 256, 128, 64 or 32. As only 8 bit depth pngs are supported, 16, 8, 4 or 2 should
/// not be used.
/// After these steps, the media should be loaded to the mcu with:
/// ´´´´uvcan --nodeid 0xD --loadmedia path/to/image.png´´´´
///
/// @note: Currently known bugs: FT81X cannot parse small Paletted PNG files (so called PNG-8 files).
/// The file size for successful parsing is somewhere 373 and 155 bytes. To be sure, dont parse
/// Paletted PNG files which are smaller than 373 bytes.
///
/// @param dest_addr: The destination address where the data is loaded in FT81X memory
/// @param exmem: The external non-volatile memory module to be used for data download
/// @param filename: The filename of the image. The file should be found from *exmem*.
uint32_t uv_ft81x_loadbitmapexmem(uv_uimedia_st *bitmap,
		uint32_t dest_addr, uv_w25q128_st *exmem, char *filename);

/// @brief: Redefinition of *uv_ft81x_loadjpgexmem* for ui-library namespace
static inline uint32_t uv_uimedia_loadbitmapexmem(uv_uimedia_st *bitmap,
		uint32_t dest_addr, uv_w25q128_st *exmem, char *filename) {
	return uv_ft81x_loadbitmapexmem(bitmap, dest_addr, exmem, filename);
}


/// @brief: Extended function to draw bitmaps
void uv_ft81x_draw_bitmap_ext(uv_uimedia_st *bitmap, int16_t x, int16_t y,
		int16_t w, int16_t h, uint32_t wrap, color_t c);


/// @brief: Draws the bitmap in (*x*, *y*) location (left-top corner)
/// stored in memory at address *addr*
///
/// @param align: Specifies which part of the image is located on (*x*, *y*) coordinates
/// @param c: Blend color. If the bitmap should be drawn withouth any blend color, give C(0xFFFFFFFF)
static inline void uv_ft81x_draw_bitmap(uv_uimedia_st *bitmap, int16_t x, int16_t y) {
	uv_ft81x_draw_bitmap_ext(bitmap, x, y, bitmap->width, bitmap->height, 0, C(0xFFFFFFFF));
}


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


/// @brief: Structure for the touchscreen transform matrix
typedef struct {
	uint32_t mat[6];
} ft81x_transfmat_st;


/// @brief: Trigger the touch screen calibration sequence
///
/// @param transform_matrix: If given, function will write the transform matrix information
/// to the address pointed by this parameter. Note that 6 words (6*4 bytes) should be
/// reserved for the transform matrix.
void uv_ft81x_touchscreen_calibrate(ft81x_transfmat_st *transform_matrix);


/// @brief: Sets the transform matrix data. This should be called by the user application
/// if such transform matrix data already exists in the non-volatile memory
void uv_ft81x_touchscreen_set_transform_matrix(ft81x_transfmat_st *transform_matrix);


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
void uv_ft81x_draw_string(char *str, ft81x_font_st *font,
		int16_t x, int16_t y, ft81x_align_e align, color_t color);


/// @brief: Returns the font height in pixels
static inline uint8_t uv_ft81x_get_font_height(ft81x_font_st *font) {
	return font->char_height;
}

/// @brief: Returns the height of string. Takes account the font height and the line count
int16_t uv_ft81x_get_string_height(char *str, ft81x_font_st *font);


/// @brief: Sets the drawing mask which masks all drawing functions to the masked area
void uv_ft81x_set_mask(int16_t x, int16_t y, uint16_t width, uint16_t height);


#endif


#endif /* UV_HAL_INC_UV_FT81X_H_ */
