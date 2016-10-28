/*
 * uv_uilist.c
 *
 *  Created on: Oct 24, 2016
 *      Author: usevolt
 */

#include <ui/uv_uilist.h>


#if CONFIG_LCD

#define this ((uv_uilist_st*)me)

#define HEIGHT_MULT			3

void uv_uilist_recalc_height(void *me) {
	if (uv_uibb(this)->height <
			uv_vector_size(&this->entries) * this->style->text_font->char_height * HEIGHT_MULT) {
		uv_uibb(this)->height =
				uv_vector_size(&this->entries) * this->style->text_font->char_height * HEIGHT_MULT;
	}
}


static void draw(void *me) {
	uint16_t i;
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	uint16_t entry_height = this->style->text_font->char_height * HEIGHT_MULT;
	for (i = 0; i < uv_vector_size(&this->entries); i++) {
		if (uv_uibb(this)->height + uv_ui_get_yglobal(this) < y + entry_height) {
			break;
		}
		color_t c = this->style->main_color;
		if (this->selected_index == i) {
			c = this->style->highlight_color;
		}
		uv_lcd_draw_rect(x, y, uv_ui_get_bb(this)->width, entry_height, c);
		uv_lcd_draw_frame(x, y, uv_uibb(this)->width, entry_height,
				this->style->frame_thickness, this->style->frame_color);
		_uv_ui_draw_text(x + uv_uibb(this)->width / 2, y + entry_height / 2,
				this->style->text_font, ALIGN_CENTER, this->style->text_color, c,
				*((char**) uv_vector_at(&this->entries, i)));
		y += entry_height - 1;
	}
}



void uv_uilist_step(void *me, uv_touch_st *touch, uint16_t step_ms) {
	if (touch->action) {

	}
	if (this->super.refresh) {
		draw(this);
	}
}


#endif

