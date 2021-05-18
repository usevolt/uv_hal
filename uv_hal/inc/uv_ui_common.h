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


#ifndef UV_UI_COMMON_H_
#define UV_UI_COMMON_H_

#include "uv_utilities.h"
#include <uv_hal_config.h>

#if CONFIG_UI


#include "ui/uv_uimedia.h"
#include "uv_spi.h"
#include "uv_w25q128.h"


/// @file: Defines the GUI drawing interface. These functions need to be implemented
/// on different targets



/// @brief: Wrapper for font data for UI library
typedef struct {
	uint16_t char_height;
	// defines the font index. Index 26-34 are anti-aliased fonts which are supported by this library
	uint8_t index;
	// handle defines the bitmap handle to be used for this font
	uint8_t handle;
} ui_font_st;
typedef ui_font_st uv_font_st;



/// @brief: FT81x library supports only anti-aliased fonts with index 26 - 34
#define UI_MAX_FONT_COUNT				9
extern ui_font_st ui_fonts[UI_MAX_FONT_COUNT];



typedef enum {
	UI_VALIGN_TOP = 0,
	UI_VALIGN_CENTER = 0x400
} ui_valign_e;
#define UI_VALIGN_MASK 		0x400



typedef enum {
	UI_HALIGN_LEFT = 0,
	UI_HALIGN_CENTER = 0x200,
	UI_HALIGN_RIGHT = 0x800
} ui_halign_e;
#define UI_HALIGN_MASK 		(0x200 | 0x800)



typedef enum {
	UI_ALIGN_LEFT_TOP = UI_HALIGN_LEFT | UI_VALIGN_TOP,
	UI_ALIGN_LEFT_CENTER = UI_HALIGN_LEFT | UI_VALIGN_CENTER,
	UI_ALIGN_RIGHT_TOP = UI_HALIGN_RIGHT | UI_VALIGN_TOP,
	UI_ALIGN_RIGHT_CENTER = UI_HALIGN_RIGHT | UI_VALIGN_CENTER,
	UI_ALIGN_CENTER = UI_HALIGN_CENTER | UI_VALIGN_CENTER,
	UI_ALIGN_CENTER_TOP = UI_HALIGN_CENTER | UI_VALIGN_TOP
} ui_align_e;



typedef enum {
	COLOR_MODE_RGB = 0,
	COLOR_MODE_GRAYSCALE
} ui_color_modes_e;



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
// Give the color as C(0xAARRGGBB)
#define C(x)		(x)



/// @brief: Returns a color which is brightened by removing *value* amount of 8-bit color
/// from all R, G and B color channels
color_t uv_uic_brighten(color_t c, int8_t value);



/// @brief: Returns a color by adding *value* amount of alpha to the given color *c*
color_t uv_uic_alpha(color_t c, int8_t value);



/// @brief: Converts *c* to grayscale and returns it
color_t uv_uic_grayscale(color_t c);



/// @brief: Returns a color which is linearily interpolated between colors ca and cb.
/// t = 0 returns ca, t = 1000 returns cb. No boundary checks are done in the calculations.
color_t uv_uic_lerpi(int32_t t, color_t ca, color_t cb);



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
	union {
		/// @brief: Width, growing to right
		int16_t width;
		int16_t w;
	};
	union {
		/// @brief: Height, growing to bottom
		int16_t height;
		int16_t h;
	};
} uv_bounding_box_st;



static inline bool uv_bb_is_null(uv_bounding_box_st *bb) {
	return bb->x == 0 &&
			bb->y == 0 &&
			bb->width == 0 &&
			bb->height == 0;
}



/// @brief: Initializes the UI LCD driver module
///
/// @return: True if touchscreen calibration was requested,
/// otherwise false.
bool uv_ui_init(void);



/// @brief: Swaps the display list buffers and makes all latest UI modifications visible
/// on the LCD display.
void uv_ui_dlswap(void);



/// @brief: Sets the backlight brightness
///
/// @param percent: The percen tof the backlight. 0 = minimum, 100 = maximum
void uv_ui_set_backlight(uint8_t percent);



/// brief: Returns the current backlight brightness
uint8_t uv_ui_get_backlight(void);



/// @brief: Clears the whole screen to color **c**
void uv_ui_clear(color_t c);



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
/// @note: Currently known bugs:
/// * FT81X cannot parse small Paletted PNG files (so called PNG-8 files).
/// The file size for successful parsing is somewhere 373 and 155 bytes. To be sure, dont parse
/// Paletted PNG files which are smaller than 373 bytes.
/// * Inkscape and Gimp seem to have problems exporting PNG's the way that FT81x can read them
/// if there isn't any semitransparent pixels in the image. This is because FT81X supports
/// png's only with alpha channel. To fix this, add some transparency into the image
///
/// @param dest_addr: The destination address where the data is loaded in FT81X memory
/// @param exmem: The external non-volatile memory module to be used for data download
/// @param filename: The filename of the image. The file should be found from *exmem*.
uint32_t uv_uimedia_loadbitmapexmem(uv_uimedia_st *bitmap,
		uint32_t dest_addr, uv_w25q128_st *exmem, char *filename);



/// @brief: Extended function to draw bitmaps
void uv_ui_draw_bitmap_ext(uv_uimedia_st *bitmap, int16_t x, int16_t y,
		int16_t w, int16_t h, uint32_t wrap, color_t c);



/// @brief: Draws the bitmap in (*x*, *y*) location (left-top corner)
/// stored in memory at address *addr*
///
/// @param align: Specifies which part of the image is located on (*x*, *y*) coordinates
/// @param c: Blend color. If the bitmap should be drawn withouth any blend color, give C(0xFFFFFFFF)
static inline void uv_ui_draw_bitmap(uv_uimedia_st *bitmap, int16_t x, int16_t y) {
	uv_ui_draw_bitmap_ext(bitmap, x, y, bitmap->width, bitmap->height, 0, C(0xFFFFFFFF));
}



/// @brief: Draws a filled circle to the screen
///
/// @param x: The X coordinate of the point's center
/// @param y: The Y coordinate of the point's center
/// @param color: The color of the point
/// @param diameter: The point diameter in pixels
void uv_ui_draw_point(int16_t x, int16_t y, color_t color, uint16_t diameter);



/// @brief: Helper function for drawing shadow points
void uv_ui_draw_shadowpoint(int16_t x, int16_t y,
		color_t color, color_t highlight_c, color_t shadow_c, uint16_t diameter);



/// @brief: Draws a filled rounded rectangle on the screen
///
/// @param x: The X coordinate of the upper left corner
/// @param y: The Y coordinate of the upper left corner
/// @param wdith: Width of the rectangle in pixels
/// @param height: Height of the rectangle in pixels
/// @param radius: The rounding radius in the corners
/// @param color: The fill color
void uv_ui_draw_rrect(const int16_t x, const int16_t y,
		const uint16_t width, const uint16_t height,
		const uint16_t radius, const color_t color);



/// @brief: helper function to draw a shadowed rounded rectangle
void uv_ui_draw_shadowrrect(const int16_t x, const int16_t y,
		const uint16_t width, const uint16_t height,
		const uint16_t radius, const color_t color,
		const color_t highlight_c, const color_t shadow_c);



/// @brief: Draws a single line on the screen
void uv_ui_draw_line(const int16_t start_x, const int16_t start_y,
		const int16_t end_x, const int16_t end_y,
		const uint16_t width, const color_t color);



typedef struct {
	int16_t x;
	int16_t y;
} uv_ui_linestrip_point_st;



typedef enum {
	UI_STRIP_TYPE_LINE = 0,
	UI_STRIP_TYPE_RIGHT,
	UI_STRIP_TYPE_LEFT,
	UI_STRIP_TYPE_ABOVE,
	UI_STRIP_TYPE_BELOW
} uv_ui_strip_type_e;



/// @brief: draws a line strip on the screen
///
/// @param points: Buffer of the point coordinates
/// @param type: The type of the strip. Refer to FT81X manual for different types.
void uv_ui_draw_linestrip(const uv_ui_linestrip_point_st *points,
		const uint16_t point_count, const uint16_t line_width, const color_t color,
		const uv_ui_strip_type_e type);



/// @brief: Structure for the touchscreen transform matrix
typedef struct {
	uint32_t mat[6];
} ui_transfmat_st;



/// @brief: Trigger the touch screen calibration sequence
///
/// @param transform_matrix: If given, function will write the transform matrix information
/// to the address pointed by this parameter. Note that 6 words (6*4 bytes) should be
/// reserved for the transform matrix.
void uv_ui_touchscreen_calibrate(ui_transfmat_st *transform_matrix);



/// @brief: Sets the transform matrix data. This should be called by the user application
/// if such transform matrix data already exists in the non-volatile memory
void uv_ui_touchscreen_set_transform_matrix(ui_transfmat_st *transform_matrix);



/// @brief: Gets the touch from the FT81X touchpanel
///
/// @return: True if the screen is touched, false if it is not touched
///
/// @param x: Pointer to where the x px coordinate will be written (or NULL)
/// @param y: Pointer to where the y px coordinate will be written (or NULL)
bool uv_ui_get_touch(int16_t *x, int16_t *y);



/// @brief: Sets the color mode for the ft81x. The mode affects  how different colors
/// are drawn on the screen.
void uv_ui_set_color_mode(ui_color_modes_e value);


ui_color_modes_e uv_ui_get_color_mode(void);



/// @brief: Sets the grayscale luminosity correction. value should be INT8_MIN + 1 ... INT8_MAX,
/// 0 is the default value. Can be used to finetune the grayscale color luminosity.
void uv_ui_set_grayscale_luminosity(int8_t value);



/// @brief: uses the FT81X co-processor to draw a text string to the display.
void uv_ui_draw_string(char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color);



/// @brief: Returns the font height in pixels
static inline uint8_t uv_ui_get_font_height(ui_font_st *font) {
	return font->char_height;
}



/// @brief: Returns the height of string. Takes account the font height and the line count
int16_t uv_ui_get_string_height(char *str, ui_font_st *font);



/// @brief: Returns the width of the string in pixels. Note that since characters are different
/// widths, the width of every character has to be read from the FT81X and this takes
/// relatively long time.
int16_t uv_ui_get_string_width(char *str, ui_font_st *font);



/// @brief: Sets the drawing mask which masks all drawing functions to the masked area
void uv_ui_set_mask(int16_t x, int16_t y, int16_t width, int16_t height);



bool uv_ui_is_visible(const int16_t x, const int16_t y,
		const int16_t width, const int16_t height);




#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN


/// @brief: Starts the GTK UI in the main thread and RTOS scheduler in another thread.
/// The function returns when the whole system is terminated. Call this instead of
/// *uv_rtos_start_scheduler* in UI applications.
///
/// @param ui_step_function: Function pointer to the step function that will be called
/// every *step_ms* ms. This task should be the one updating the GUI in the system,
/// since GTK requires all drawing functions to be executed in the main thread.
void uv_ui_rtos_start_scheduler(void);


#endif

#endif

#endif /* UV_UI_COMMON_H_ */
