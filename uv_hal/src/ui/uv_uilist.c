/*
 * uv_uilist.c
 *
 *  Created on: Oct 24, 2016
 *      Author: usevolt
 */

#include <ui/uv_uilist.h>


#if CONFIG_UI

#define this ((uv_uilist_st*)me)

static void draw(void *me, const uv_bounding_box_st *pbb);
static void touch(void *me, uv_touch_st *touch);


void uv_uilist_init(void *me, char **buffer, uint16_t buffer_len, const uv_uistyle_st *style) {
	uv_uiobject_init(me);
	this->selected_index = -1;
	uv_vector_init(&this->entries, buffer, buffer_len, sizeof(char*));
	this->style = style;
	((uv_uiobject_st*) this)->step_callb = &uv_uilist_step;
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
	int16_t y = thisy;
	uint16_t entry_height = CONFIG_UI_LIST_ENTRY_HEIGHT;
	int16_t sely = 0;

	if (this->selected_index >= uv_vector_size(&this->entries)) {
		this->selected_index = uv_vector_size(&this->entries) - 1;
	}


	for (i = 0; i < uv_vector_size(&this->entries); i++) {
		if ((uv_uibb(this)->height + thisy) < (y + entry_height)) {
			break;
		}
		if (this->selected_index == i) {
			sely = y;
		}
		else {
#if CONFIG_LCD
			uv_lcd_draw_mrect(x, y, uv_ui_get_bb(this)->width, entry_height, this->style->inactive_bg_c,
					pbb);

			uv_lcd_draw_mframe(x, y, uv_uibb(this)->width, entry_height, 1, this->style->inactive_frame_c,
					pbb);

			_uv_ui_draw_mtext(x + uv_uibb(this)->width / 2, y + entry_height / 2,
					this->style->font, ALIGN_CENTER, this->style->inactive_font_c,
					this->style->inactive_bg_c, *((char**) uv_vector_at(&this->entries, i)), 1.0f, pbb);
#elif CONFIG_FT81X
			uv_ft81x_draw_shadowrrect(x, y, uv_uibb(this)->width, entry_height, CONFIG_UI_RADIUS,
					this->style->inactive_bg_c, this->style->highlight_c, this->style->shadow_c);
			uv_ft81x_draw_string(*((char**) uv_vector_at(&this->entries, i)), this->style->font->index,
					x + uv_uibb(this)->width / 2, y + entry_height / 2, ALIGN_CENTER,
					this->style->inactive_font_c);
#endif
		}
		y += entry_height - 1;
	}
	if (this->selected_index >= 0) {
#if CONFIG_LCD
		uv_lcd_draw_mrect(x, sely, uv_ui_get_bb(this)->width, entry_height, this->style->active_bg_c,
				pbb);

		uv_lcd_draw_mframe(x, sely, uv_uibb(this)->width, entry_height, 1, this->style->active_frame_c,
				pbb);

		_uv_ui_draw_mtext(x + uv_uibb(this)->width / 2, sely + entry_height / 2,
				this->style->font, ALIGN_CENTER, this->style->active_font_c,
				this->style->active_bg_c,
				*((char**) uv_vector_at(&this->entries, this->selected_index)), 1.0f, pbb);
#elif CONFIG_FT81X
		uv_ft81x_draw_shadowrrect(x, sely, uv_uibb(this)->width, entry_height, CONFIG_UI_RADIUS,
				this->style->active_bg_c, this->style->highlight_c, this->style->shadow_c);
		uv_ft81x_draw_string(*((char**) uv_vector_at(&this->entries, this->selected_index)),
				this->style->font->index, x + uv_uibb(this)->width / 2, sely + entry_height / 2,
				ALIGN_CENTER, this->style->active_font_c);
#endif
	}

}



uv_uiobject_ret_e uv_uilist_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (this->super.refresh) {
		((uv_uiobject_st*) this)->vrtl_draw(this, pbb);
		this->super.refresh = false;
		ret = UIOBJECT_RETURN_REFRESH;
	}

	return ret;
}

static void touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		if (touch->y <= uv_vector_size(&this->entries) * CONFIG_UI_LIST_ENTRY_HEIGHT) {
			this->selected_index = touch->y / CONFIG_UI_LIST_ENTRY_HEIGHT;
			// prevent touch action propagating to other elements
			touch->action = TOUCH_NONE;
			uv_ui_refresh(this);
		}
	}
}


/// @brief: Pushes a new element into the end of the list
void uv_uilist_push_back(void *me, char *str) {
	uv_vector_push_back(&this->entries, (void*) &str);
	uv_uilist_recalc_height(this);
	uv_ui_refresh_parent(this);
}

/// @brief: Pops the last element from the list
void uv_uilist_pop_back(void *me, char *dest) {
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



#endif

