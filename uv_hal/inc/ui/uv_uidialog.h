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


#ifndef UV_HAL_INC_UI_UV_UIDIALOG_H_
#define UV_HAL_INC_UI_UV_UIDIALOG_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_ui.h"


#if CONFIG_UI

/// @file: UIDialog represents a non-permanent pop-up screen which is shown
/// on top of everything else for a some time period.
/// It has it's own event loop and it handles the same functions as a uidisplay, only
/// for a smaller subsystem.


typedef struct {
	EXTENDS(uv_uidisplay_st);

} uv_uidialog_st;


/// @brief: Initializes the uidialog
void uv_uidialog_init(void *me, uv_uiobject_st **object_array, const uv_uistyle_st* style);


/// @brief: Executes the uidialog. This function returns only when
/// the step function assigned to this uidialog returns UIDIALOG_RETURN_FINISHED.
void uv_uidialog_exec(void *me);


/// @brief: Adds an object into uidialog
static inline void uv_uidialog_add(void *me, void *object,
		int16_t x, int16_t y, uint16_t width, uint16_t height) {
	uv_uidisplay_add(me, object, x, y, width, height);
}


/// @brief: Clears the uidialog from objects
static inline void uv_uidialog_clear(void *me) {
	uv_uidisplay_clear(me);
}


static inline void uv_uidialog_set_transparent(void *me, bool value) {
	uv_uiwindow_set_transparent(me, value);
}


static inline void 	uv_uidialog_set_stepcallback(void *me, uv_uiobject_ret_e (*step_callb)(uint16_t)) {
	uv_uiwindow_set_stepcallback(me, step_callb);
}


#endif

#endif /* UV_HAL_INC_UI_UV_UIDIALOG_H_ */
