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


void uv_uitabwindow_init(void *me, int16_t tab_count,
		const uv_uistyle_st *style,
		uv_uiobject_st **obj_array,
		const char **tab_names) {
	uv_uiwindow_init(this, obj_array, style);
	this->active_tab = 0;
	this->tab_count = tab_count;
	this->tab_names = tab_names;
}


static void draw(void *me) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t tab_w = 0;
	int16_t active_tab_x = 0;
	int16_t active_tab_w = 0;

	for (int16_t i = 0; i < this->tab_count; i++) {
		tab_w = uv_ui_text_width_px((char *)this->tab_names[i], this->super.style->font, 1.0f) + 10;
		if (tab_w < CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH) tab_w = CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH;
		if (this->active_tab != i) {

			uv_lcd_draw_rect(x, y, tab_w, CONFIG_UI_TABWINDOW_HEADER_HEIGHT, this->super.style->inactive_bg_c);
			uv_lcd_draw_frame(x, y, tab_w + 1, CONFIG_UI_TABWINDOW_HEADER_HEIGHT,
					1, this->super.style->inactive_frame_c);
			_uv_ui_draw_text(x + 5, y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT / 2, this->super.style->font,
					ALIGN_CENTER_LEFT, this->super.style->inactive_font_c, C(0xFFFFFFFF),
					(char*) this->tab_names[i], 1.0f);
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
			C(0xFFFFFFFF), (char *) this->tab_names[this->active_tab], 1.0f);

}


void uv_uitabwindow_step(void *me, uv_touch_st *touch, uint16_t step_ms) {
	bool refresh = this->super.super.refresh;

	uv_uiwindow_step(this, touch, step_ms);

	// todo: tab changing. When tab has been changed, this->tab_changed has to be true for 1 step cycle
	this->tab_changed = false;
	if (touch->action == TOUCH_CLICKED) {
		if (touch->y <= CONFIG_UI_TABWINDOW_HEADER_HEIGHT) {
			int16_t total_w = 0;
			for (int16_t i = 0; i < this->tab_count; i++) {
				int16_t tab_w = uv_ui_text_width_px((char *) this->tab_names[i], this->super.style->font, 1.0f) + 10;
				if (tab_w < CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH) {
					tab_w = CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH;
				}
				if (touch->x < total_w + tab_w) {
					this->active_tab = i;
					this->tab_changed = true;
					refresh = true;
					// prevent touch action from propagating further
					touch->action = TOUCH_NONE;
					break;
				}
				total_w += tab_w;
			}
		}
	}

	if (refresh) {
		draw(this);
	}

}
#endif
