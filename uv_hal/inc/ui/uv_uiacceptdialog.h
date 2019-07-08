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


#ifndef INC_UIACCEPTDIALOG_H_
#define INC_UIACCEPTDIALOG_H_


#include <uv_utilities.h>
#include <uv_ui.h>

#if CONFIG_UI


/// @file: uiacceptdialog is a quick one-function-call way of showing
/// a full-screen acceptance window. It consists of a informative
/// text and two buttons, which are used to accept or not accept it.
/// The user's choice is shown as a return value.

#define UIACCEPTDIALOG_BUFFER_LEN		4

/// @brief: UIACCEPTDIALOG return values
typedef enum {
	UIACCEPTDIALOG_RET_NO = 0,
	UIACCEPTDIALOG_RET_YES
} uv_uiacceptdialog_ret_e;


/// @brief: Displays a save dialog screen
typedef struct {
	uv_uidialog_st dialog;
	uv_uiobject_st *buffer[UIACCEPTDIALOG_BUFFER_LEN];

	uv_uilabel_st info_label;
	uv_uibutton_st yes_button;
	uv_uibutton_st no_button;

	uv_uiacceptdialog_ret_e ret;
} uv_uiacceptdialog_st;


uv_uiacceptdialog_ret_e uv_uiacceptdialog_exec(uv_uiacceptdialog_st *this,
		char *info_str, char *yes_str, char *no_str, const uv_uistyle_st *style);

#endif /* INC_UIACCEPTDIALOG_H_ */


#endif
