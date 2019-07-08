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

#ifndef UV_HAL_INC_UI_UV_UIMEDIABUTTON_H_
#define UV_HAL_INC_UI_UV_UIMEDIABUTTON_H_


#include <uv_hal_config.h>
#include "uv_ui.h"


#if CONFIG_UI


/// @brief: The media alignment
typedef enum {
	/// Media is shown on left side of the text
	UIMEDIABUTTON_ALIGN_HORIZONTAL = 0,
	// media is shown on top of the text
	UIMEDIABUTTON_ALIGN_VERTICAL
} uimediabutton_alignment_e;


/// @brief: uimediabutton is a button with a media (i.e. an image) placed next to the text
typedef struct __attribute__((packed)) {
	EXTENDS(uv_uibutton_st);

	uv_uimedia_st *media;
	uimediabutton_alignment_e align;

} uv_uimediabutton_st;

#ifdef this
#undef this
#endif
#define this ((uv_uimediabutton_st*)me)


/// @brief: Initializes the mediabutton
///
/// @param media: The media file which will be shown on this uimediabutton
void uv_uimediabutton_init(void *me, char *text,
		uv_uimedia_st *media, const uv_uistyle_st *style);


static inline bool uv_uimediabutton_clicked(void *me) {
	return uv_uibutton_clicked(this);
}

static inline bool uv_uimediabutton_is_down(void *me) {
	return uv_uibutton_is_down(this);
}

/// @brief: Sets the button text
static inline void uv_uimediabutton_set_text(void *me, char *text) {
	uv_uibutton_set_text(me, text);
}

void uv_uimediabutton_set_media(void *me, uv_uimedia_st *media);

/// @brief: Returns the button text
static inline char *uv_uimediabutton_get_text(void *me) {
	return uv_uibutton_get_text(me);
}


/// @brief: Sets the button alignment
static inline void uv_uimediabutton_set_align(void *me, uimediabutton_alignment_e value) {
	this->align = value;
}

/// @brief: Gets the alignment
static inline uimediabutton_alignment_e uv_uimediabutton_get_align(void *me) {
	return this->align;
}



#undef this


#endif

#endif /* UV_HAL_INC_UI_UV_UIMEDIABUTTON_H_ */
