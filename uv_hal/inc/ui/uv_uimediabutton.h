/*
 * uv_uimediabutton.h
 *
 *  Created on: Nov 1, 2016
 *      Author: usevolt
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
typedef struct {
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
