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

#include "uv_ui.h"
#include "uv_rtos.h"


#if CONFIG_UI

#define this ((uv_uidialog_st*) me)




void uv_uidialog_init(void *me, uv_uiobject_st **object_array, const uv_uistyle_st* style) {

	uv_uidisplay_init(this, object_array, style);
	// uidialog uses uiwindow's draw function
	uv_uiobject_set_draw_callb(this, &uv_uiwindow_draw);
	// dialog defaults to non-transparent.
	uv_uidialog_set_transparent(this, false);
	// dialog uses same draw callback as windows
	uv_uiobject_set_draw_callb(this, &uv_uiwindow_draw);
}


void uv_uidialog_exec(void *me) {
	while (true) {
		int16_t step_ms = 20;

		uv_uiobject_ret_e ret;
		ret = uv_uidisplay_step(this, step_ms);

		if (ret & UIOBJECT_RETURN_KILLED) {
			break;
		}

		uv_rtos_task_delay(step_ms);
	}
}


















#endif
