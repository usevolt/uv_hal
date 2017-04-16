/*
 * uv_uitreewindow.h
 *
 *  Created on: Apr 12, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UITREEVIEW_H_
#define UV_HAL_INC_UI_UV_UITREEVIEW_H_


#include <uv_hal_config.h>
#include "uv_ui.h"
#include "uv_lcd.h"

#if CONFIG_LCD


#if !defined(CONFIG_UI_TREEVIEW_ITEM_HEIGHT)
#error "CONFIG_UI_TREEVIEW_ITEM_HEIGHT should define the tree view object's header's height in pixels"
#endif

/// @brief: Object structure which will be shown as a single treewindow object
typedef struct {
	/// @brief: Visible name of the object
	const char *name;
	/// @brief: uiwindow object which is shown when this object is opened
	uv_uiwindow_st *window;
	/// @brief: Function callback for showing this object's window
	void (*show_callb)(void);
} uv_uitreeobject_st;


typedef struct {
	EXTENDS(uv_uiwindow_st);

	/// @brief: Tells the current active (opened) object
	int16_t active_object;
	/// @brief: Tells the object count
	uint16_t obj_count;
	/// @brief: Pointer to the object array
	const uv_uitreeobject_st *object_array;
	const uv_font_st *arrow_font;

} uv_uitreeview_st;



/// @brief: Initializes a treeview. A Treeview contains cascadeable objects
/// In a tree-kind of way.
///
/// @param object_array: Pointer to an array of child structures which contain a pointer
/// to the window structure and it's name.
/// @param arrow_font: Pointer to the font used to draw "arrows" in front of the obejcts names
void uv_uitreeview_init(void *me, const uv_uitreeobject_st *object_array, const int16_t object_count,
		const uv_font_st *arrow_font, const uv_uistyle_st * style);


/// @brief: Step function which is called internally
bool _uv_uitreeview_step(void *me, uv_touch_st *touch, uint16_t step_ms,
		const uv_bounding_box_st *pbb);

void uv_uitreeview_set_active(void *me, uint16_t active_index);

#endif


#endif /* UV_HAL_INC_UI_UV_UITREEVIEW_H_ */
