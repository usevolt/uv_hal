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

#include <ui/uv_uilist.h>

#if CONFIG_UI

#define this ((uv_uilist_st*)me)

static void touch(void *me, uv_touch_st *touch);
static void draw(void *me, const uv_bounding_box_st *pbb);
static void draw_entry(void *me, void *entry, bool selected, uv_bounding_box_st *bb);


void uv_uilist_init(void *me, uv_uilist_entry_st *buffer,
		uint16_t buffer_len, const uv_uistyle_st *style) {
	uv_uiobject_init(me);
	this->selected_index = -1;
	uv_vector_init(&this->entries, buffer, buffer_len, sizeof(uv_uilist_entry_st));
	((uv_uiobject_st*) this)->step_callb = &uv_uilist_step;
	this->align = ALIGN_CENTER;
	this->clicked = false;
	this->bg_c = style->bg_c;
	this->text_c = style->text_color;
	this->font = style->font;
	this->draw_entry = &draw_entry;
	uv_uiobject_set_draw_callb(this, &draw);
	uv_uiobject_set_touch_callb(this, &touch);
}


void uv_uilist_recalc_height(void *me) {
	if (uv_uibb(this)->height <
			uv_vector_size(&this->entries) * CONFIG_UI_LIST_ENTRY_HEIGHT) {
		uv_uibb(this)->height =
				uv_vector_size(&this->entries) * CONFIG_UI_LIST_ENTRY_HEIGHT;
	}
}


static void draw(void *me, const uv_bounding_box_st *pbb) {
	uint16_t i;
	int16_t x = uv_ui_get_xglobal(this);
	int16_t thisy = uv_ui_get_yglobal(this);
	uint16_t entry_height = CONFIG_UI_LIST_ENTRY_HEIGHT;
	int16_t y;
	valignment_e valign = uv_ui_get_valignment(this->align);
	if (valign == VALIGN_CENTER) {
		y = thisy + uv_uibb(this)->height / 2 -
				(entry_height * uv_vector_size(&this->entries) / 2);
	}
	else {
		y = thisy;
	}

	if (this->selected_index >= uv_vector_size(&this->entries)) {
		this->selected_index = uv_vector_size(&this->entries) - 1;
	}


	uv_bounding_box_st bb;
	uv_bounding_box_st selected_bb;
	for (i = 0; i < uv_vector_size(&this->entries); i++) {
		bb.x = x;
		bb.y = y;
		bb.width = uv_uibb(this)->width;
		bb.height = entry_height;
		if ((uv_uibb(this)->height + thisy) < (y + entry_height)) {
			break;
		}
		if (this->selected_index != i) {
			if (this->draw_entry) {
				this->draw_entry(this, uv_uilist_at(this, i), false, &bb);
			}
		}
		else {
			selected_bb = bb;
		}
		y += entry_height - 1;
	}
	if (this->selected_index != -1) {
		if (this->draw_entry) {
			this->draw_entry(this, uv_uilist_at(this, this->selected_index), true, &selected_bb);
		}
	}
}

static void draw_entry(void *me, void *entry, bool selected, uv_bounding_box_st *bb) {
	color_t highlight_c = uv_uic_brighten(this->bg_c, selected ? 120 : 80);
	color_t shadow_c = uv_uic_brighten(this->bg_c, selected ? 0 : -80);
	color_t c = uv_uic_brighten(this->bg_c, selected ? 30 : 0);
	uv_uilist_entry_st *uilist_entry = entry;

	uv_ui_draw_shadowrrect(bb->x, bb->y, bb->width, bb->height, CONFIG_UI_RADIUS,
			c, highlight_c, shadow_c);
	uv_ui_draw_string(uilist_entry->str, this->font,
			bb->x + bb->width / 2, bb->y + bb->height / 2, ALIGN_CENTER,
			this->text_c);
}





uv_uiobject_ret_e uv_uilist_step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	this->clicked = false;

	return ret;
}

static void touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		int16_t index = -1;
		int16_t y;
		if (uv_ui_get_valignment(this->align) == VALIGN_CENTER) {
			y = (touch->y - ((uv_uibb(this)->height -
				CONFIG_UI_LIST_ENTRY_HEIGHT * uv_vector_size(&this->entries)) / 2));
		}
		else {
			y = touch->y;
		}
		if (y >= 0) {
			index = y / CONFIG_UI_LIST_ENTRY_HEIGHT;
		}

		if ((index >= 0) &&
				(index < uv_vector_size(&this->entries))) {
			this->clicked = true;
			this->selected_index = index;
			uv_ui_refresh(this);
			// prevent touch action propagating to other elements
			touch->action = TOUCH_NONE;
		}
	}
}


/// @brief: Pushes a new element into the end of the list
void uv_uilist_push_back(void *me, uv_uilist_entry_st *str) {
	uv_vector_push_back(&this->entries, str);
	uv_uilist_recalc_height(this);
	uv_ui_refresh_parent(this);
}

/// @brief: Pops the last element from the list
void uv_uilist_pop_back(void *me, uv_uilist_entry_st *dest) {
	uv_vector_pop_back(&this->entries, &dest);
	uv_uilist_recalc_height(this);
	uv_ui_refresh_parent(this);
}


/// @brief: Removes a *index*'th entry from the list
void uv_uilist_remove(void *me, uint16_t index) {
	uv_vector_remove(&this->entries, index, 1);
	uv_uilist_recalc_height(this);
	uv_ui_refresh_parent(this);
}


void uv_uilist_insert(void *me, uint16_t index, char *str) {
	uv_vector_insert(&this->entries, index, &str);
	uv_uilist_recalc_height(this);
	uv_ui_refresh_parent(this);
}

void uv_uilist_clear(void *me) {
	uv_vector_clear(&this->entries);
	uv_uilist_recalc_height(this);
	uv_ui_refresh_parent(this);
}


void uv_uilist_select(void *me, int16_t index) {
	if (this->selected_index != index) {
		uv_ui_refresh(this);
	}
	this->selected_index = index;
}



#endif

