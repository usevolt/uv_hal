/*
 * uv_uitabwindow.h
 *
 *  Created on: Oct 29, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UITABWINDOW_H_
#define UV_HAL_INC_UI_UV_UITABWINDOW_H_


#include <uv_hal_config.h>
#include <uv_ui.h>
#include <uv_lcd.h>


#if CONFIG_LCD


#if !CONFIG_UI_TABWINDOW_HEADER_HEIGHT
#error "CONFIG_UI_TABWINDOW_HEADER_HEIGHT should define the tabwindow header height in pixels"
#endif
#if !CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH
#error "CONFIG_UI_TABWINDOW_HEADER_MIN_WIDTH should define the tabwindow header minimum width in pixels"
#endif


/// @brief: tab window is a window with different tabs
typedef struct {
	EXTENDS(uv_uiwindow_st);

	/// @brief: tells how many tabs this tabwindow has
	uint16_t tab_count;
	/// @brief: Pointers to the tab names
	const char **tab_names;
	/// @brief: Index of the current active tab
	uint16_t active_tab;
	/// @brief: True for 1 step cycle when the tab was changed
	bool tab_changed;
} uv_uitabwindow_st;

#ifndef this
#undef this
#endif
#define this ((uv_uitabwindow_st*)me)


/// @brief: Initializes the tab window
void uv_uitabwindow_init(void *me, int16_t tab_count,
		const uv_uistyle_st *style,
		uv_uiobject_st **obj_array,
		const char **tab_names);


static inline bool uv_uitabwindow_tab_changed(void *me) {
	return this->tab_changed;
}


static inline void uv_uitabwindow_set_tab(void *me, int16_t tab_index) {
	uv_ui_refresh(this);
	if (tab_index < this->tab_count) this->active_tab = tab_index;
}


static inline int16_t uv_uitabwindow_tab(void *me) {
	return this->active_tab;
}


/// @brief: Step function is called from the owner window
void uv_uitabwindow_step(void *me, uv_touch_st *touch, uint16_t step_ms);


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UITABWINDOW_H_ */
