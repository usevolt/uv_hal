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

/// @brief: Object structure which will be shown as a single treewindow object
typedef struct {
	/// @brief: Visible name of the object
	const char *name;
	/// @brief: uiwindow object which is shown when this object is opened
	uv_uiwindow_st *window;
} uv_uitreeobject_st;


typedef struct {
	EXTENDS(uv_uiwindow_st);

	/// @brief: Tells the current active (opened) object
	int16_t active_object;
	/// @brief: Tells the object count
	uint16_t obj_count;

	uv_uitreeobject_st **object_array;
	uv_vector_st objects;

} uv_uitreeview_st;



/// @brief: Initializes a treeview. A Treeview contains cascadeable objects
/// In a tree-kind of way.
///
/// @param array: Pointer to an array of child structures which contain a pointer
/// to the window structure and it's name.
void uv_uitreeview_init(void *me, uv_uitreeobject_st **object_array, int16_t object_count,
		const uv_uistyle_st * style);


/// @brief: Step function which is called internally
bool _uv_uitreeview_step(void *me, uv_touch_st *touch, uint16_t step_ms,
		const uv_bounding_box_st *pbb);

/// @brief: Adds a new object to the end of the list
uv_errors_e uv_uitreeview_push_back(void *me, uv_uitreeobject_st *object);

/// @brief: Adds a new object to the start of the list
uv_errors_e uv_uitreeview_push_front(void *me, uv_uitreeobject_st *object);


#endif


#endif /* UV_HAL_INC_UI_UV_UITREEVIEW_H_ */
