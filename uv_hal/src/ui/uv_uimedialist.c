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

#include <ui/uv_uimedialist.h>


#if CONFIG_UI

#define this ((uv_uimedialist_st*)me)

static void draw_entry(void *me, void *entry, bool selected, uv_bounding_box_st *bb);




void uv_uimedialist_init(void *me, uv_uimedialist_entry_st *buffer,
		uint16_t buffer_len, const uv_uistyle_st *style) {
	uv_uilist_init(me,  (uv_uilist_entry_st*) buffer, buffer_len, style);
	uv_uilist_set_entry_size(this, sizeof(uv_uimedialist_entry_st));
	uv_uilist_set_entry_draw(this, &draw_entry);
}





static void draw_entry(void *me, void *entry, bool selected, uv_bounding_box_st *bb) {
	color_t highlight_c = uv_uic_brighten(((uv_uilist_st*) this)->bg_c, selected ? 60 : 30);
	color_t shadow_c = uv_uic_brighten(((uv_uilist_st*) this)->bg_c, selected ? 0 : -30);
	color_t c = uv_uic_brighten(((uv_uilist_st*) this)->bg_c, selected ? 30 : 0);
	uv_uimedialist_entry_st *uilist_entry = entry;

	uv_ft81x_draw_shadowrrect(bb->x, bb->y, bb->width, bb->height, CONFIG_UI_RADIUS,
			c, highlight_c, shadow_c);

	int16_t offset = 10;
	int16_t imgw = 0, imgh = 0;
	if (uilist_entry->bitmap != NULL) {
		imgw = uv_uimedia_get_bitmapwidth(uilist_entry->bitmap);
		imgh = uv_uimedia_get_bitmapheight(uilist_entry->bitmap);
		uv_ft81x_draw_bitmap(uilist_entry->bitmap, bb->x + offset, bb->y + imgh / 2);
	}
	uv_ft81x_draw_string(((uv_uilist_entry_st*) uilist_entry)->str, ((uv_uilist_st*) this)->font,
			bb->x + offset * 2 + imgw, bb->y + bb->height / 2, ALIGN_CENTER_LEFT,
			((uv_uilist_st*) this)->text_c);

}





#endif

