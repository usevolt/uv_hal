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

#ifndef UV_HAL_INC_UI_UV_UIFRAMEWINDOW_H_
#define UV_HAL_INC_UI_UV_UIFRAMEWINDOW_H_


#include <uv_hal_config.h>
#include <uv_ui.h>
#include <uv_lcd.h>


#if CONFIG_UI


/// @brief: The margin in pixels from the window edges to the thin frame line,
/// applied on all four edges. Configurable; defaults to 5.
#if !defined(CONFIG_UI_FRAMEWINDOW_FRAME_MARGIN)
#define CONFIG_UI_FRAMEWINDOW_FRAME_MARGIN		5
#endif


/// @brief: A window that draws a thin (1px) rectangular frame inset from its
/// edges by CONFIG_UI_FRAMEWINDOW_FRAME_MARGIN pixels. An optional title rests on
/// the top frame line in the top-left corner; the line is broken (not drawn)
/// underneath the title text.
///
/// Works like uv_uiwindow_st (objects are added to it the same way), but the
/// frame eats some space from the edges, so position child objects relative to
/// uv_uiframewindow_get_content_bb() rather than the window's full bounding box.
typedef struct {
	EXTENDS(uv_uiwindow_st);

	/// @brief: Title drawn on the top frame line, or NULL / "" for no title. The
	/// caller-owned string must outlive the window.
	char *title;
	/// @brief: Colour of the frame line.
	color_t frame_c;
	/// @brief: Colour of the title text.
	color_t text_c;
	uv_font_st *font;
} uv_uiframewindow_st;

#ifdef this
#undef this
#endif
#define this ((uv_uiframewindow_st*)me)


/// @brief: Initializes the frame window. *obj_array* holds pointers to the child
/// objects, exactly like uv_uiwindow_init.
void uv_uiframewindow_init(void *me, uv_uiobject_st **obj_array,
		const uv_uistyle_st *style);


/// @brief: Sets the title drawn on the top frame line (NULL or "" for none).
/// Recomputes the content bounding box, so call this before adding children.
void uv_uiframewindow_set_title(void *me, char *title);


static inline char *uv_uiframewindow_get_title(void *me) {
	return this->title;
}


/// @brief: Sets the colour of the frame line.
static inline void uv_uiframewindow_set_frame_color(void *me, color_t c) {
	this->frame_c = c;
	uv_ui_refresh(this);
}


/// @brief: Sets the colour of the title text.
static inline void uv_uiframewindow_set_text_color(void *me, color_t c) {
	this->text_c = c;
	uv_ui_refresh(this);
}


/// @brief: Returns the bounding box of the area inside the frame, i.e. the area
/// available for child objects. Mirrors uv_uitabwindow_get_contentbb.
uv_bounding_box_st uv_uiframewindow_get_content_bb(void *me);


/// @brief: implementation of uv_uiwindow's add function
static inline void uv_uiframewindow_addxy(void *me, void *object,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	uv_uiwindow_addxy(me, object, x, y, width, height);
}

static inline void uv_uiframewindow_add(void *me, void *object,
		uv_bounding_box_st *bb) {
	uv_uiwindow_add(me, object, bb);
}

/// @brief: Clears the window's children while keeping the frame draw callback.
void uv_uiframewindow_clear(void *me);


static inline void uv_uiframewindow_set_stepcallb(void *me,
		uv_uiobject_ret_e (*step)(void *, const uint16_t), void *user_ptr) {
	uv_uiwindow_set_stepcallback(me, step, user_ptr);
}


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIFRAMEWINDOW_H_ */
