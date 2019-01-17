/*
 * uv_uitabwindow.c
 *
 *  Created on: Oct 29, 2016
 *      Author: usevolt
 */


#include <string.h>
#include "ui/uv_uitabwindow.h"

#if CONFIG_UI

#define this ((uv_uitabwindow_st*)me)

static void draw(void *me, const uv_bounding_box_st *pbb);
static void touch(void *me, uv_touch_st *touch);


void uv_uitabwindow_init(void *me, int16_t tab_count,
		const uv_uistyle_st *style,
		uv_uiobject_st **obj_array,
		const char **tab_names) {
	uv_uiwindow_init(this, obj_array, style);
	uv_uiwindow_set_content_bb_default_pos(this, 0, CONFIG_UI_TABWINDOW_HEADER_HEIGHT);
	this->font = style->font;
	this->text_c = style->text_color;
	this->active_tab = 0;
	this->tab_count = tab_count;
	this->tab_names = tab_names;
	((uv_uiobject_st*) this)->step_callb = &uv_uitabwindow_step;
	uv_uiobject_set_draw_callb(this, &draw);
	uv_uiobject_set_touch_callb(this, &touch);
}


static void draw(void *me, const uv_bounding_box_st *pbb) {

	// super draw function
	uv_uiwindow_draw(this, pbb);

	int16_t thisx = uv_ui_get_xglobal(this);
	int16_t x = thisx;
	int16_t y = uv_ui_get_yglobal(this);
	int16_t tab_w = 0;
	int16_t active_tab_x = 0;
	int16_t active_tab_w = 0;


	for (int16_t i = 0; i < this->tab_count; i++) {
		tab_w = uv_ui_text_width_px((char *)this->tab_names[i], this->font, 1.0f) + 10;
		if (tab_w < CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH) tab_w = CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH;
		if (this->active_tab != i) {
#if CONFIG_LCD
			uv_lcd_draw_mrect(x, y, tab_w, CONFIG_UI_TABWINDOW_HEADER_HEIGHT,
					this->super.style->inactive_bg_c, pbb);
			uv_lcd_draw_mframe(x, y, tab_w + 1, CONFIG_UI_TABWINDOW_HEADER_HEIGHT,
					1, this->super.style->inactive_frame_c, pbb);
			_uv_ui_draw_mtext(x + 5, y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT / 2, this->super.style->font,
					ALIGN_CENTER_LEFT, this->super.style->inactive_font_c, C(0xFFFFFFFF),
					(char*) this->tab_names[i], 1.0f, pbb);
#elif CONFIG_FT81X
			uv_ft81x_draw_shadowrrect(x, y, tab_w, CONFIG_UI_TABWINDOW_HEADER_HEIGHT,
					CONFIG_UI_RADIUS, ((uv_uiwindow_st*) this)->bg_c,
					uv_uic_brighten(((uv_uiwindow_st*) this)->bg_c, 30),
					uv_uic_brighten(((uv_uiwindow_st*) this)->bg_c, -30));
			uv_ft81x_draw_string((char*) this->tab_names[i], this->font,
					x + x, y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT / 2, ALIGN_CENTER_LEFT,
					this->text_c);
#endif
		}
		else {
			active_tab_x = x;
			active_tab_w = tab_w;
		}
		x += tab_w;
	}
#if CONFIG_LCD
	// draw horizontal line
	uv_lcd_draw_mrect(x, y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT - 1,
			thisx + uv_uibb(this)->width - x,
			1, this->super.style->inactive_frame_c, pbb);

	// lastly draw active tab
	uv_lcd_draw_mrect(active_tab_x, y, active_tab_w, CONFIG_UI_TABWINDOW_HEADER_HEIGHT,
			this->super.style->active_bg_c, pbb);
	uv_lcd_draw_mrect(active_tab_x, y, active_tab_w, 1,
			this->super.style->active_frame_c, pbb);
	uv_lcd_draw_mrect(active_tab_x, y, 1, CONFIG_UI_TABWINDOW_HEADER_HEIGHT,
			this->super.style->active_frame_c, pbb);
	uv_lcd_draw_mrect(active_tab_x + active_tab_w, y, 1, CONFIG_UI_TABWINDOW_HEADER_HEIGHT,
			this->super.style->active_frame_c, pbb);
	_uv_ui_draw_mtext(active_tab_x + 5, y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT / 2, this->super.style->font,
			ALIGN_CENTER_LEFT, this->super.style->active_font_c,
			C(0xFFFFFFFF), (char *) this->tab_names[this->active_tab], 1.0f, pbb);
#elif CONFIG_FT81X
	// draw horizontal line
	uv_ft81x_draw_line(thisx, y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT - 1,
			thisx + uv_uibb(this)->width,
			y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT- 1, 1,
			uv_uic_brighten(((uv_uiwindow_st*) this)->bg_c, 30));
	// draw active tab
	uv_ft81x_draw_shadowrrect(active_tab_x, y, active_tab_w, CONFIG_UI_TABWINDOW_HEADER_HEIGHT,
			CONFIG_UI_RADIUS, uv_uic_brighten(((uv_uiwindow_st*) this)->bg_c, 20),
			uv_uic_brighten(((uv_uiwindow_st*) this)->bg_c, 30),
			uv_uic_brighten(((uv_uiwindow_st*) this)->bg_c, -30));
	uv_ft81x_draw_string((char*) this->tab_names[this->active_tab],
			this->font, active_tab_x + 5,
			y + CONFIG_UI_TABWINDOW_HEADER_HEIGHT / 2, ALIGN_CENTER_LEFT,
			this->text_c);

#endif

}

uv_bounding_box_st uv_uitabwindow_get_contentbb(void *me) {
	uv_bounding_box_st bb = *uv_uibb(me);
	bb.y += CONFIG_UI_TABWINDOW_HEADER_HEIGHT;
	bb.height -= CONFIG_UI_TABWINDOW_HEADER_HEIGHT;
	return bb;
}



uv_uiobject_ret_e uv_uitabwindow_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	ret = uv_uiwindow_step(this, step_ms, pbb);

	// When tab has been changed, this->tab_changed has to be true for 1 step cycle
	this->tab_changed = false;
	return ret;
}


static void touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		if (touch->y <= CONFIG_UI_TABWINDOW_HEADER_HEIGHT) {
			int16_t total_w = 0;
			for (int16_t i = 0; i < this->tab_count; i++) {
				int16_t tab_w = uv_ui_text_width_px((char *) this->tab_names[i], this->font, 1.0f) + 10;
				if (tab_w < CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH) {
					tab_w = CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH;
				}
				if (touch->x < total_w + tab_w) {
					this->active_tab = i;
					this->tab_changed = true;
					uv_ui_refresh(this);
					// prevent touch action from propagating further
					touch->action = TOUCH_NONE;
					break;
				}
				total_w += tab_w;
			}
		}
	}
}

#endif
