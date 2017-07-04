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
	EXTENDS(uv_uiwindow_st);
	/// @brief: Visible name of the object
	const char *name;
	/// @brief: Function callback for showing this object's window
	void (*show_callb)(void);
	/// @brief: Marks that this object is active (opened)
	bool active;
} uv_uitreeobject_st;


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





/// @brief: Initializes uitreeobject. This should be called for all uitreeviewobjects which
/// are not defined as constants.
void uv_uitreeobject_init(void *me, uv_uiobject_st **object_array,
		const char *name, void (*show_callb)(void), const uv_uistyle_st* style);


/// @brief: Adds a uiwindow to be shown in the uitreeobject. This should be called
/// in a uitreeiobject's show-callback. Only one window can be visible in a uitreeoject.
static inline void uv_uitreeobject_add(void *me, uv_uiobject_st* obj,
		int16_t x, int16_t y, uint16_t width, uint16_t height) {
	uv_uiwindow_add(me, obj, x, y, width, height);
}






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
void uv_uitreeview_set_active(void *me, uv_uitreeobject_st *obj);


/// @brief: By default only 1 object can be active (== open) at one time
static inline void uv_uitreeview_set_oneactive(void *me, bool value) {
	((uv_uitreeview_st*) me)->one_active = value;
	uv_ui_refresh(me);
}


static inline void uv_uitreeview_set_stepcallb(void *me,
		uv_uiobject_ret_e (*step)(const uint16_t step_ms)) {
	uv_uiwindow_set_stepcallback(me, step);
}



/// @brief: Adds a new obect to the treeview
///
/// @param object: Pointer to the object to be added
void uv_uitreeview_add(void *me, uv_uitreeobject_st *object);

#endif


#endif /* UV_HAL_INC_UI_UV_UITREEVIEW_H_ */
