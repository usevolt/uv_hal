/*
 * uv_uitabwindow.c
 *
 *  Created on: Oct 29, 2016
 *      Author: usevolt
 */


#include <string.h>
#include "ui/uv_uitabwindow.h"

#if CONFIG_LCD

#define this ((uv_uitabwindow_st*)me)


static void draw(void *me) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t tab_w = 0;
	int16_t active_tab_x = 0;
	int16_t active_tab_w = 0;

	for (int16_t i = 0; i < this->tab_count; i++) {
		tab_w = uv_ui_text_width_px((char *)this->tab_names[i], this->super.style->font) + 10;
		if (tab_w < CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH) tab_w = CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH;
		if (this->active_tab != i) {

			uv_lcd_draw_rect(x, y, tab_w, CONFIG_UI_TABWINDOW_HEADER_HEIGHT, this->super.style->inactive_bg_c);
			uv_lcd_draw_frame(x, y, tab_w + 1, CONFIG_UI_TABWINDOW_HEADER_HEIGHT,
					1, this->super.style->inactive_frame_c);
			_uv_ui_draw_text(x + 5, y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT / 2, this->super.style->font,
					ALIGN_CENTER_LEFT, this->super.style->inactive_font_c, C(0xFFFFFFFF), (char*) this->tab_names[i]);
		}
		else {
			active_tab_x = x;
			active_tab_w = tab_w;
		}
		x += tab_w;
	}
	// draw horizontal line
	uv_lcd_draw_rect(x, y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT - 1,
			uv_ui_get_xglobal(this) + uv_uibb(this)->width - x,
			1, this->super.style->inactive_frame_c);

	// lastly draw active tab
	uv_lcd_draw_rect(active_tab_x, y,
			active_tab_w, CONFIG_UI_TABWINDOW_HEADER_HEIGHT, this->super.style->active_bg_c);
	uv_lcd_draw_rect(active_tab_x, y,
			active_tab_w, 1, this->super.style->active_frame_c);
	uv_lcd_draw_rect(active_tab_x, y, 1, CONFIG_UI_TABWINDOW_HEADER_HEIGHT, this->super.style->active_frame_c);
	uv_lcd_draw_rect(active_tab_x + active_tab_w, y, 1, CONFIG_UI_TABWINDOW_HEADER_HEIGHT,
			this->super.style->active_frame_c);
	_uv_ui_draw_text(active_tab_x + 5, y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT / 2, this->super.style->font,
			ALIGN_CENTER_LEFT, this->super.style->active_font_c,
			C(0xFFFFFFFF), (char *) this->tab_names[this->active_tab]);

}


void uv_uitabwindow_step(void *me, uv_touch_st *touch, uint16_t step_ms) {
	bool refresh = this->super.super.refresh;

	uv_uiwindow_step(this, touch, step_ms);

	if (refresh) {
		if (this->tab_change_callb) {
			this->tab_change_callb(this, this->active_tab);
		}
		draw(this);
	}

}
#endif
