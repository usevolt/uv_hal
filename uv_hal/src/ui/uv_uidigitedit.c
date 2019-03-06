/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/



#include "ui/uv_uidigitedit.h"


#if CONFIG_UI

static uv_uiobject_ret_e uv_uidigitedit_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb);
static void draw(void *me, const uv_bounding_box_st *pbb);
static void touch(void *me, uv_touch_st *touch);



#define this ((uv_uidigitedit_st *) me)


void uv_uidigitedit_init(void *me, uv_font_st *font,
		color_t color, uint32_t value, const uv_uistyle_st *style) {
	uv_uilabel_init(this, font, ALIGN_CENTER, color, "");
	this->style = style;

	uv_uiobject_set_step_callb(this, &uv_uidigitedit_step);
	uv_uiobject_set_draw_callb(this, &draw);
	uv_uiobject_set_touch_callb(this, &touch);
	this->value = !value;
	this->changed = false;
	this->numpaddialog_title = "";
	uv_uidigitedit_set_value(this, value);

}


void uv_uidigitedit_set_value(void *me, uint32_t value) {
	if (this->value != value) {
		sprintf(this->str, "%u", (unsigned int) value);
		uv_uilabel_set_text(this, this->str);
		this->changed = true;
	}
	this->value = value;
}


static void draw(void *me, const uv_bounding_box_st *pbb) {
	uint16_t x = uv_ui_get_xglobal(this),
			y = uv_ui_get_yglobal(this);

	uint16_t height = uv_ft81x_get_string_height(((uv_uilabel_st*) this)->str,
			((uv_uilabel_st*) this)->font) +
					uv_ft81x_get_font_height(((uv_uilabel_st*) this)->font);
	if (height > uv_uibb(this)->height) {
		height = uv_uibb(this)->height;
	}
	else {
		y += (uv_uibb(this)->height - height) / 2;
	}
	uv_ft81x_draw_shadowrrect(x, y, uv_uibb(this)->width, height,
			CONFIG_UI_RADIUS, C(0xFFFFFFFF), C(0xFFDDDDDD), C(0xFFBBBBBB));

	_uv_uilabel_draw(this, pbb);
}


static uv_uiobject_ret_e uv_uidigitedit_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	uv_uilabel_step(this, step_ms, pbb);

	this->changed = false;

	return ret;
}



static void touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		touch->action = TOUCH_NONE;
		uint32_t value = uv_uinumpaddialog_exec(
				this->numpaddialog_title, this->value, this->style);
		uv_uidigitedit_set_value(this, value);
		uv_ui_refresh(this);
	}
}



#endif
