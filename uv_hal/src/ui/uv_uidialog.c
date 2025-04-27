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

#include "uv_ui.h"
#include "uv_rtos.h"


#if CONFIG_UI

#define this ((uv_uidialog_st*) me)



static void draw(void *me, const uv_bounding_box_st *pbb) {
	uv_uiwindow_draw(this, pbb);

	// draw all the objects added to the screen
	_uv_uiwindow_draw_children(this, pbb);

	uv_uidisplay_draw_touch_ind(this);

	// all UI components should now be updated, swap display list buffers
	uv_ui_dlswap();
}



void uv_uidialog_init(void *me, uv_uiobject_st **object_array, const uv_uistyle_st* style) {

	uv_uidisplay_init(this, object_array, style);
	// dialog defaults to non-transparent.
	uv_uidialog_set_transparent(this, false);
	uv_uiobject_set_draw_callb(this, &draw);
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
