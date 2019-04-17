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


#ifndef UV_HAL_INC_UI_UV_UIDIGITEDIT_H_
#define UV_HAL_INC_UI_UV_UIDIGITEDIT_H_

#include "uv_utilities.h"
#include "uv_ui.h"


#if CONFIG_UI


/// @brief: Structure for showing text label on the screen.
typedef struct {
	EXTENDS(uv_uilabel_st);

	char str[16];
	char *title;
	char *numpaddialog_title;
	color_t bg_color;
	uint32_t value;
	bool changed;
	const uv_uistyle_st *style;
	int32_t limit_max;
} uv_uidigitedit_st;


#undef this
#define this ((uv_uidigitedit_st*)me)


void uv_uidigitedit_init(void *me, uv_font_st *font,
		color_t color, uint32_t value, const uv_uistyle_st *style);



/// @brief: Set's the text of all objects which are inherited from uv_uilabel_st
///
/// @note: Since string are passed as a reference parameters, updating the same text
/// will not refresh the label itself. Thus uv_ui_refresh should be called after updating the text.
void uv_uidigitedit_set_value(void *me, uint32_t value);


/// @brief: Sets the color of the label text
static inline void uv_uidigitedit_set_text_color(void *me, color_t c) {
	uv_uilabel_set_color(me, c);
}

static inline void uv_uidigitedit_set_bg_color(void *me, color_t c) {
	this->bg_color = c;
}


/// @brief: Sets the title for the digitedit. The title text is shown below the digitedit fiel
static inline void uv_uidigitedit_set_title(void *me, char *value) {
	this->title = value;
}

static inline char *uv_uidigitedit_get_title(void *me) {
	return this->title;
}

/// @brief: Sets the string for the numpad dialog which opens when the uidigitedit is clicked
static inline void uv_uidigitedit_set_numpad_title(void *me, char *value) {
	this->numpaddialog_title = value;
}

/// @brief: Returns true on the step cycle when the value was changed
static inline bool uv_uidigitedit_value_changed(void *me) {
	return this->changed;
}

static inline uint32_t uv_uidigitedit_get_value(void *me) {
	return this->value;
}


/// @brief: Sets the maximum limit
static inline void uv_uidigitedit_set_maxlimit(void *me, int32_t value) {
	this->limit_max = value;
}

/// @brief: Returns the maximum limit
static inline int32_t uv_uidigitedit_get_maxlimit(void *me) {
	return this->limit_max;
}



#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIDIGITEDIT_H_ */
