/*
 * uv_ubutton.h
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
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
typedef struct {
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
static inline void uv_uiimage_set_blendc(void *me, color_t c) {
	this->blend_c = c;
}

/// @brief: returns the blend color
static inline color_t uv_uiimage_get_blendc(void *me) {
	return this->blend_c;
}



#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIIMAGE_H_ */
