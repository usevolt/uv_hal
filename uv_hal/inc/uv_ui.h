/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#ifndef UW_UI_H_
#define UW_UI_H_

#include "ui/uv_uimedia.h"
#include "ui/uv_uibutton.h"
#include "ui/uv_uitogglebutton.h"
#include "ui/uv_uiduallabelbutton.h"
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
#include "ui/uv_uicheckbox.h"
#include "ui/uv_uiacceptdialog.h"
#include "ui/uv_uimedialist.h"
#include "ui/uv_uigraph.h"
#include "ui/uv_uivalveslider.h"
#include <stdarg.h>


/// @brief: Variadic language function. Returns a pointer to a string
/// in language given by *lang_index*. The following arguments
/// should be strings in different languages. ///
/// @example: // This returns "English"
///			uv_uitr(0, "English", "Finnish", "Swedish");
char *uv_uitr(uint8_t lang_index, ...);



#endif /* UW_UI_H_ */
