/*
 * uv_label.c
 *
 *  Created on: Sep 30, 2016
 *      Author: usevolt
 */



#include "uv_ui.h"
#include "uv_utilities.h"

#if CONFIG_UI

#define this ((uv_uilabel_st*)me)

	void uv_uilabel_init(void *me, uv_font_st *font,
			alignment_e alignment, color_t color, char *str) {
	uv_uiobject_init(this);
	this->font = font;
	this->str = str;
	this->align = alignment;
	this->color = color;
	uv_uiobject_set_draw_callb(this, &_uv_uilabel_draw);
	uv_uiobject_set_step_callb(this, &uv_uilabel_step);
}





void _uv_uilabel_draw(void *me, const uv_bounding_box_st *pbb) {
	uint16_t x = uv_ui_get_xglobal(this),
			y = uv_ui_get_yglobal(this);

	if ((this->align == ALIGN_CENTER) ||
			(this->align == ALIGN_TOP_CENTER)) {
		x += uv_uibb(this)->width / 2;
	}
	else if ((this->align == ALIGN_CENTER_RIGHT) ||
			(this->align == ALIGN_TOP_RIGHT)) {
		x += uv_uibb(this)->width;
	}
	if ((this->align == ALIGN_CENTER) ||
			(this->align == ALIGN_CENTER_LEFT) ||
			(this->align == ALIGN_CENTER_RIGHT)) {
		y += uv_uibb(this)->height / 2;
	}
	uv_ft81x_draw_string(this->str, this->font, x, y, this->align, this->color);

}

uv_uiobject_ret_e uv_uilabel_step(void *me, uint16_t step_ms, const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (((uv_uiobject_st*) this)->refresh) {
		_uv_uiobject_draw(this, pbb);
	}

	return ret;
}



void uv_uilabel_set_text(void *me, char *str) {
	if (strcmp(this->str, str) != 0) {
		this->str = str;
		uv_ui_refresh(me);
	}
}


/// @brief: Sets the color of the label text
void uv_uilabel_set_color(void *me, color_t c) {
	if (this->color != c) {
		this->color = c;
		uv_ui_refresh(me);
	}
}




int16_t uv_ui_text_height_px(char *str, const uv_font_st *font, float scale) {
	int16_t line_count = 1;
	char *c = str;
	if (!str || *c == '\0') {
		return 0;
	}
	while (*c != '\0') {
		if (*c == '\n' || *c == '\r') {
			if (*(c+1) != '\n' && *(c+1) != '\r') {
				line_count++;
			}
		}
		c++;
	}
	return line_count * font->char_height * scale;
}


#undef this
#define this ((uv_uidigit_st*)me)


/// @brief: Initializes the digit label.
	void uv_uidigit_init(void *me, uv_font_st *font,
			alignment_e alignment, color_t color, char *format, int value) {
		uv_uilabel_init(me, font, alignment, color, "");
	this->divider = 1;
	strcpy(this->format, format);
	// force redraw
	uv_uidigit_set_value(this, !value);
	uv_uidigit_set_value(this, value);
	((uv_uiobject_st*) this)->step_callb = &uv_uidigit_step;
}


void uv_uidigit_set_value(void *me, int value) {
	if (this->value != value) {
		uv_uilabel_set_text(me, "");
		this->value = value;
		int val = value / (this->divider);
		unsigned int cval = abs(value) % (this->divider);

		if (this->divider != 1) {
			sprintf(this->str, this->format, val, cval);
		}
		else {
			sprintf(this->str, this->format, val);
		}
		this->super.str = this->str;
		uv_ui_refresh(this);
	}
}


void uv_uidigit_set_text(void *me, char *str) {
	strcpy(this->str, str);
	uv_uilabel_set_text(this, str);
}




#endif




