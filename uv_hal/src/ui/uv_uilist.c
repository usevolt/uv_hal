/*
 * uv_uilist.c
 *
 *  Created on: Oct 24, 2016
 *      Author: usevolt
 */

#include <ui/uv_uilist.h>


#if CONFIG_LCD

#define this ((uv_uilist_st*)me)


void uv_uilist_init(void *me, char **buffer, uint16_t buffer_len, const uv_uistyle_st *style) {
	uv_uiobject_init(me);
	this->selected_index = -1;
	uv_vector_init(&this->entries, buffer, buffer_len, sizeof(char*));
	this->style = style;
}


void uv_uilist_recalc_height(void *me) {
	if (uv_uibb(this)->height <
			uv_vector_size(&this->entries) * CONFIG_UI_LIST_ENTRY_HEIGHT) {
		uv_uibb(this)->height =
				uv_vector_size(&this->entries) * CONFIG_UI_LIST_ENTRY_HEIGHT;
	}
}


static void draw(void *me) {
	uint16_t i;
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	uint16_t entry_height = CONFIG_UI_LIST_ENTRY_HEIGHT;
	int16_t sely = 0;
	for (i = 0; i < uv_vector_size(&this->entries); i++) {
		if (uv_uibb(this)->height + uv_ui_get_yglobal(this) < y + entry_height) {
			break;
		}
		if (this->selected_index == i) {
			sely = y;
			y += entry_height - 1;
			continue;
		}
		uv_lcd_draw_rect(x, y, uv_ui_get_bb(this)->width, entry_height, this->style->inactive_bg_c);

		uv_lcd_draw_frame(x, y, uv_uibb(this)->width, entry_height, 1, this->style->inactive_frame_c);

		_uv_ui_draw_text(x + uv_uibb(this)->width / 2, y + entry_height / 2,
				this->style->font, ALIGN_CENTER, this->style->inactive_font_c,
				this->style->inactive_bg_c, *((char**) uv_vector_at(&this->entries, i)));
		y += entry_height - 1;
	}
	if (this->selected_index >= 0) {
		uv_lcd_draw_rect(x, sely, uv_ui_get_bb(this)->width, entry_height, this->style->active_bg_c);

		uv_lcd_draw_frame(x, sely, uv_uibb(this)->width, entry_height, 1, this->style->active_frame_c);

		_uv_ui_draw_text(x + uv_uibb(this)->width / 2, sely + entry_height / 2,
				this->style->font, ALIGN_CENTER, this->style->active_font_c,
				this->style->active_bg_c, *((char**) uv_vector_at(&this->entries, this->selected_index)));
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

