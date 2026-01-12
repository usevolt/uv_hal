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


#ifndef HAL_UV_HAL_INC_UI_UV_UIMEDIA_H_
#define HAL_UV_HAL_INC_UI_UV_UIMEDIA_H_

#include <uv_hal_config.h>
#include "uv_utilities.h"

#if CONFIG_UI

/// @brief: Enum of different uimedia types
enum {
	UV_UIMEDIA_IMAGE = 0,
	UV_UIMEDIA_AUDIO
};
typedef uint8_t uv_uimedia_types_e;



/// @brief: An uimedia structure. Defines the basic
/// properties for a downloaded media files, such as graphics and audio
typedef struct {
	/// @brief: The size of the bitmap in bytes
	uint32_t size;
	uv_uimedia_types_e type;
	// Name of the file name which should be loaded to the graphics RAM memory.
	// This is used only if the media file should be loaded from external memory
	char *filename;
	// union of file type dependent properties
	union {
		struct {
			/// @brief: the width of the image in pixels
			uint16_t width;
			/// @brief: The height of the image in pixels
			uint16_t height;
			/// @brief: The image format of the image. Baseline jpgs are in BITMAP_FORMAT_RGB565,
			/// and png8 images with transparency are in BITMAP_FORMAT_PALETTED4444 format.
			/// Note that if png8 images are used without transparency, the format should
			/// explicitly set to BITMAP_FORMAT_PALETTED565 by the user after loading the image.
			uint8_t format;
			// for PALETTED images, the palette size specifies the palette size in bytes.
			// Palette resides in the *addr* address, and the image starts right after it
			uint16_t palette_size;
		};

	};
	union {
		/// @brief: The address of the image in FT81X media RAM. For CONFIG_UI_FT81X only
		uint32_t addr;
		/// @brief: The pointer to the address of the image in FT81X media RAM,
		/// or a pointer to cairo_surface_t on simulator target. For CONFIG_UI_X11 only
		void *surface_ptr;
	};
} uv_uimedia_st;


/// @brief: returns the filename of the media file
static inline char *uv_uimedia_get_filename(uv_uimedia_st *this) {
	return this->filename;
}

/// @brief: Returns the type of the initialized media file
static inline uv_uimedia_types_e uv_uimedia_get_type(uv_uimedia_st *this) {
	return this->type;
}

/// @brief: Returns the address where the bitmap is located
static inline uint32_t uv_uimedia_get_address(uv_uimedia_st *this) {
	return this->addr;
}


/// @brief: Returns the end address of the image. A new media file can be
/// loaded right to this address
static inline uint32_t uv_uimedia_get_end_addr(uv_uimedia_st *this) {
	return this->addr + this->size;
}

/// @brief: Returns the width of the bitmap in pixels
static inline uint16_t uv_uimedia_get_bitmapwidth(uv_uimedia_st *this) {
	return (this == NULL) ? 0 : this->width;
}


/// @brief: Returns the height of the bitmap in pixels
static inline uint16_t uv_uimedia_get_bitmapheight(uv_uimedia_st *this) {
	return (this == NULL) ? 0 : this->height;
}

/// @brief: Returns the size of the bitmap in bytes
static inline uint32_t uv_uimedia_get_size(uv_uimedia_st *this) {
	return (this == NULL) ? 0 : this->size;
}

#endif

#endif /* HAL_UV_HAL_INC_UI_UV_UIMEDIA_H_ */
