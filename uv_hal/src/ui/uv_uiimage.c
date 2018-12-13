/*
 * ubutton.c
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */


#include <ui/uv_uiimage.h>
#include "ui/uv_uilabel.h"

#if CONFIG_UI


static inline void draw(void *me, const uv_bounding_box_st *pbb);
static uv_uiobject_ret_e uv_uiimage_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb);


#define this ((uv_uiimage_st*)me)

void uv_uiimage_init(void *me, uv_uimedia_st *media,
		uiimage_wrap_e wrap, alignment_e align) {
	uv_uiobject_init(me);
	this->media = media;
	this->wrap = wrap;
	this->align = align;
	this->blend_c = C(0xFFFFFFFF);

	uv_uiobject_set_draw_callb(this, &draw);
	((uv_uiobject_st*) this)->step_callb = &uv_uiimage_step;
}



static inline void draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;

	int16_t mw = this->media->width;
	int16_t mh = this->media->height;

	if (this->align == ALIGN_CENTER ||
			this->align == ALIGN_TOP_CENTER) {
		x += w / 2 - mw / 2;
	}
	if (this->align == ALIGN_CENTER_RIGHT ||
			this->align == ALIGN_TOP_RIGHT) {
		x += w - mw;
	}
	if (this->align == ALIGN_CENTER ||
			this->align == ALIGN_CENTER_LEFT ||
			this->align == ALIGN_CENTER_RIGHT) {
		y += h / 2 - mh / 2;
	}

	uv_ft81x_draw_bitmap_ext(this->media, x, y, mw, mh, this->wrap, this->blend_c);
}

static uv_uiobject_ret_e uv_uiimage_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (uv_ui_get_enabled(this)) {
		if (((uv_uiobject_st*) this)->refresh) {
			((uv_uiobject_st*) this)->vrtl_draw(this, pbb);
			((uv_uiobject_st*) this)->refresh = false;
			ret = UIOBJECT_RETURN_REFRESH;
		}
	}

	return ret;
}



#endif
