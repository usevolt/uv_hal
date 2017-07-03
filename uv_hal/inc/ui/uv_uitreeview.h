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
	/// @brief: Marks that this object is active (opened)
	bool active;
	/// @brief: Object's width and height are stored here. This way same uiwindow_st
	/// structure can be used for multiple childs if ony one is active at the time.
	int16_t width;
	int16_t height;
} uv_uitreeobject_st;


/// @brief: Initializes uitreeobject. This should be called for all uitreeviewobjects which
/// are not defined as constants.
void uv_uitreeobject_init(void *me, const char *name,
		uv_uiwindow_st *window, void (*show_callb)(void));


typedef struct {
	EXTENDS(uv_uiwindow_st);

	/// @brief: Tells the object count
	uint16_t obj_count;
	/// @brief: If true (default), only one object can be active at the time.
	/// When this is true, it's possible to define the objects in a union,
	/// since only one will be active.
	bool one_active;
	/// @brief: Pointer to the object array
	uv_uitreeobject_st **object_array;
	/// @brief: Font for showing the arrow indicators
	const uv_font_st *arrow_font;

} uv_uitreeview_st;



/// @brief: Initializes a treeview. A Treeview contains cascadeable objects
/// In a tree-kind of way.
///
/// @param object_array: Pointer to an array of child structures which contain a pointer
/// to the window structure and it's name.
/// @param arrow_font: Pointer to the font used to draw "arrows" in front of the objects names
void uv_uitreeview_init(void *me, uv_uitreeobject_st ** const object_array,
		const uv_font_st *arrow_font, const uv_uistyle_st * style);


/// @brief: Step function which is called internally
uv_uiobject_ret_e _uv_uitreeview_step(void *me, uv_touch_st *touch, uint16_t step_ms,
		const uv_bounding_box_st *pbb);

/// @brief: Sets the currently active object
void uv_uitreeview_set_active(void *me, uint16_t active_index);


/// @brief: By default only 1 object can be active (== open) at one time
static inline void uv_uitreeview_set_oneactive(void *me, bool value) {
	((uv_uitreeview_st*) me)->one_active = value;
	uv_ui_refresh(me);
}


/// @brief: Adds a new obect to the treeview
///
/// @param object: Pointer to the object to be added
/// @param width: Object's window's desired width in pixels
/// @param height: Object's window's desired height in pixels
void uv_uitreeview_add(void *me, uv_uitreeobject_st *object, int16_t width, int16_t height);

#endif


#endif /* UV_HAL_INC_UI_UV_UITREEVIEW_H_ */
