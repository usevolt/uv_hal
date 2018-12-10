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
	uint16_t size;
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
		};

	};
	/// @brief: The address of the image in FT81X media RAM
	uint32_t addr;
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


/// @brief: Returns the width of the bitmap in pixels
static inline uint16_t uv_uimedia_get_bitmapwidth(uv_uimedia_st *this) {
	return this->width;
}


/// @brief: Returns the height of the bitmap in pixels
static inline uint16_t uv_uimedia_get_bitmapheight(uv_uimedia_st *this) {
	return this->height;
}

/// @brief: Returns the size of the bitmap in bytes
static inline uint32_t uv_uimedia_get_size(uv_uimedia_st *this) {
	return this->size;
}

#endif

#endif /* HAL_UV_HAL_INC_UI_UV_UIMEDIA_H_ */
