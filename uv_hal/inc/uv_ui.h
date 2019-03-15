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


#ifndef UW_UI_H_
#define UW_UI_H_

#include "ui/uv_uimedia.h"
#include "ui/uv_uibutton.h"
#include "ui/uv_uitogglebutton.h"
#include "ui/uv_uidisplay.h"
#include "ui/uv_uilabel.h"
#include "ui/uv_uislider.h"
#include "ui/uv_uiwindow.h"
#include "ui/uv_ui_styles.h"
#include "ui/uv_uilist.h"
#include "ui/uv_uikeyboard.h"
#include "ui/uv_uitabwindow.h"
#include "ui/uv_uiprogressbar.h"
#include "ui/uv_uilayout.h"
#include "ui/uv_uitoucharea.h"
#include "ui/uv_uitreeview.h"
#include "ui/uv_uitransition.h"
#include "ui/uv_uidialog.h"
#include "ui/uv_uinumpad.h"
#include "ui/uv_uidigitedit.h"
#include "ui/uv_uiimage.h"
#include "ui/uv_uimediabutton.h"
#include "ui/uv_uilistbutton.h"
#include <stdarg.h>


/// @brief: Variadic language function. Returns a pointer to a string
/// in language iven with *lang_index*. The following arguments
/// should be strings in different languages. The number of strings should
/// match the number of languages set with *uv_ui_set_lang_count*.
///
/// @example: // This returns "English"
///			uv_uitr(0, "English", "Finnish", "Swedish");
char *uv_uitr(uint8_t lang_index, ...);



#endif /* UW_UI_H_ */
