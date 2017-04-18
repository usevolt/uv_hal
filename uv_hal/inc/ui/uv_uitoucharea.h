/*
 * uv_uitoucharea.h
 *
 *  Created on: Jan 18, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UITOUCHAREA_H_
#define UV_HAL_INC_UI_UV_UITOUCHAREA_H_


#include <ui/uv_uiobject.h>
#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_lcd.h"
#include "ui/uv_ui_styles.h"

/// @file: uv_uitoucharea is an invisible area wich registers the touchscreen actions done
/// over it.

#if CONFIG_LCD


typedef struct {
	EXTENDS(uv_uiobject_st);

	uv_touch_st touch;
} uv_uitoucharea_st;


void uv_uitoucharea_init(void *me);

uv_uiobject_ret_e uv_uitoucharea_step(void *me, uv_touch_st *touch,
		uint16_t step_ms, const uv_bounding_box_st *pbb);


#if defined(this)
#undef this
#endif
#define this ((uv_uitoucharea_st*)me)


/// @brief: Returns true if the touch area is pressed. If *x* and *y* are not NULL,
/// the touch local coordinates are stored to them.
bool uv_uitoucharea_pressed(void *me, int16_t *x, int16_t *y);

/// @brief: Returns true if the touch area is released. If *x* and *y* are not NULL,
/// the touch local coordinates are stored to them.
bool uv_uitoucharea_released(void *me, int16_t *x, int16_t *y);

/// @brief: Returns true if the touch area is held down. If *x* and *y* are not NULL,
/// the touch local coordinates are stored to them.
bool uv_uitoucharea_is_down(void *me, int16_t *x, int16_t *y);

/// @brief: Returns true if the touch area was clicked. If *x* and *y* are not NULL,
/// the touch local coordinates are stored to them.
bool uv_uitoucharea_clicked(void *me, int16_t *x, int16_t *y);


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UITOUCHAREA_H_ */
