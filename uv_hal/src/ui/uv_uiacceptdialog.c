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



#include <uv_uiacceptdialog.h>


#if CONFIG_UI

static uv_uiobject_ret_e step(void *user_ptr, uint16_t step_ms);


uv_uiacceptdialog_ret_e uv_uiacceptdialog_exec(uv_uiacceptdialog_st *this,
		char *info_str, char *yes_str, char *no_str, const uv_uistyle_st *style) {
	this->ret = UIACCEPTDIALOG_RET_NO;

	uv_uidialog_init(&this->dialog, this->buffer, style);
	uv_uidialog_set_stepcallback(&this->dialog, &step, this);

	uv_uigridlayout_st grid;
	uv_uigridlayout_init(&grid, 0, 0, uv_uibb(&this->dialog)->width,
			uv_uibb(&this->dialog)->height, 2, 3);
	uv_bounding_box_st bb = uv_uigridlayout_next(&grid);

	uv_uilabel_init(&this->info_label, style->font, ALIGN_CENTER,
			C(0xFFFFFFFF), info_str);
	uv_uidialog_addxy(&this->dialog, &this->info_label,
			bb.x, bb.y, bb.width * 2, bb.height * 2);

	bb = uv_uigridlayout_next(&grid);
	bb = uv_uigridlayout_next(&grid);
	bb = uv_uigridlayout_next(&grid);

	bb = uv_uigridlayout_next(&grid);
	uv_uibutton_init(&this->yes_button, yes_str, style);
	uv_uidialog_add(&this->dialog, &this->yes_button, &bb);

	bb = uv_uigridlayout_next(&grid);
	uv_uibutton_init(&this->no_button, no_str, style);
	uv_uidialog_add(&this->dialog, &this->no_button, &bb);

	uv_uidialog_exec(&this->dialog);

	return this->ret;
}


static uv_uiobject_ret_e step(void *user_ptr, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;
	uv_uiacceptdialog_st *this = user_ptr;

	if (uv_uibutton_clicked(&this->yes_button)) {
		this->ret = UIACCEPTDIALOG_RET_YES;
		ret = UIOBJECT_RETURN_KILLED;
	}
	else if (uv_uibutton_clicked(&this->no_button)) {
		this->ret = UIACCEPTDIALOG_RET_NO;
		ret = UIOBJECT_RETURN_KILLED;
	}
	else {

	}

	return ret;
}

#endif
