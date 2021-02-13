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

#ifndef UV_HAL_INC_UI_UV_UIIMAGE_H_
#define UV_HAL_INC_UI_UV_UIIMAGE_H_

#include <uv_hal_config.h>
#include "ui/uv_uiobject.h"
#include "ui/uv_uilabel.h"
#include "uv_utilities.h"

#if CONFIG_UI





/// @brief: Defines the wrapping possibilities for the images. Only affects the drawn output
/// if scale is set to UV_UIIMAGE_TRUE_SIZE
enum {
	// Wrapping is disabled, the bitmap is shown as-is
	UIIMAGE_WRAP_BORDER = 0,
	// If the bitmap was smaller than uiimage, the image is wrapped from the edges to fill the uiimage
	UIIMAGE_WRAP_REPEAT
};
typedef uint8_t uiimage_wrap_e;



/// @brief: Image structure
typedef struct __attribute__((packed)) {
	EXTENDS(uv_uiobject_st);

	// pointer to the media structure which should be loaded to media RAM externally,
	// i.e. uv_uiimage structure doesnt take care of loading the media.
	uv_uimedia_st *media;

	// defines the wrapping properties for the image. Only affects if scale is UIIMAGE_TRUE_SIZE.
	uiimage_wrap_e wrap;
	// The alignment of the bitmap. Only affects if scale is UIIMAGE_TRUE_SIZE and bitmap
	// is smaller than uiimage.
	alignment_e align;

	// The blending color for the image
	color_t blend_c;

} uv_uiimage_st;

#ifdef this
#undef this
#endif
#define this ((uv_uiimage_st*)me)



/// @brief: Initializes the image
///
/// @param media: Pointer to the media structure which defines the image shown here
void uv_uiimage_init(void *me, uv_uimedia_st *media,
		uiimage_wrap_e wrap, alignment_e align);



/// @brief: Sets the blend color for the image. If set to C(0xFFFFFFFF), bitmap is drawn
/// without any blend color
void uv_uiimage_set_blendc(void *me, color_t c);

/// @brief: returns the blend color
static inline color_t uv_uiimage_get_blendc(void *me) {
	return this->blend_c;
}



#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIIMAGE_H_ */
