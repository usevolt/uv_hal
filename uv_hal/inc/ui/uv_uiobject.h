/*
 * uv_uobject.h
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UIOBJECT_H_
#define UV_HAL_INC_UI_UV_UIOBJECT_H_


#include <uv_hal_config.h>
#include "ui/uv_ui_styles.h"
#include "uv_utilities.h"
#if CONFIG_LCD
#include "uv_lcd.h"
#elif CONFIG_FT81X
#include "uv_ft81x.h"
#endif

#if CONFIG_UI




/// @brief: Lists all different touchscreen actions
enum {
	/// @brief: Represents a null touch event (does nothing). If any
	/// object wouldn't want the touch event to propagate futher to other
	/// objects, it should change it's touch event to TOUCH_NONE after
	/// executing it. Uv_uiwindow_st notifies this and cancels the propagation
	/// of the event.
	TOUCH_NONE = 0,
	/// @brief: User pressed down the touchscreen
	/// x and y variables contain the local coordinates of the press
	TOUCH_PRESSED,
	/// @brief: Will be triggered after TOUCH_PRESSED until releasing press
	/// or starting the dragging.
	TOUCH_IS_DOWN,
	/// @brief: User released from the touchscreen
	/// x and y variables contain the local coordinates of the release
	TOUCH_RELEASED,
	/// @brief: User clicked on the touchscreen without moving
	/// x and y variables contain the local coordinates of the click
	TOUCH_CLICKED,
	/// @brief: User is dragging the finger on the touchscreen
	/// x and y variables contain the drag offset from the last step cycle
	TOUCH_DRAG
};
typedef uint8_t uv_touch_action_e;





/// @brief: Touchscreen touch structure. All object's receive their
/// touching actions via this structure as a step function parameter.
typedef struct {
	/// @brief: Defines the action type
	uv_touch_action_e action;
	/// @brief: local X coordinate. Contains different information depending on the action.
	int16_t x;
	/// @brief: local Y coordinate. Contains different information depending on the action.
	int16_t y;
} uv_touch_st;


/// @brief: Return values for step functions
enum {
	/// @brief: Default value. Step functions should return this if
	/// the object is still valid and no refreshing needed
	UIOBJECT_RETURN_ALIVE = 0,
	/// @rief: If object refreshed itself, it should return this
	UIOBJECT_RETURN_REFRESH = (1 << 0),
	/// @brief: To be used with uiwindow's application step callbacks.
	/// If this is returned, the whole step fuction call is propagated to the
	/// parent object and step function is terminated. This can be used when switching
	/// window structures which are using unions to save memory.
	UIOBJECT_RETURN_KILLED = (1 << 1)
};
typedef uint8_t uv_uiobject_ret_e;


/// @brief: Typedef of uv_uiwindow. uiobject has a pointer to it's parent,
/// which is of type uv_uiwindow_st.
typedef struct _uv_uiwindow_st uv_uiwindow_st;
typedef struct _uv_uitransition_st uv_uitransition_st;

/// @brief: Main struct for GUI object structure. Every GUI object should declare
/// this struct as it's first variable, e.g. they should inherit from this.
typedef struct uv_uiobject_st {
	/// @brief: Object bounding box
	uv_bounding_box_st bb;
	/// @brief: Pointer to the object's parent or NULL
	uv_uiwindow_st *parent;
	/// @brief: Pointer to the object's step-function
	/// @param this: Pointer to this object
	/// @param step_ms: The step cycle duration in milliseconds
	/// @param touch: Touchscreen structure which holds the touchscreen actions made on this object
	uv_uiobject_ret_e (*step_callb)(void *this, uint16_t step_ms,
			const uv_bounding_box_st *pbb);
	/// @brief: Virtual drawing function. This will be called anytime when
	/// the object needs redrawing.
	void (*vrtl_draw)(void *me, const uv_bounding_box_st *pbb);
	/// @brief: Virtual touch function. This will be called when the object
	/// is clicked. This is separate from the step function because objects are
	/// drawn back-to-front, but touches are processed front-to-back.
	void (*vrtl_touch)(void *me, uv_touch_st *touch);
	/// @brief: Object will be rendered on the screen only if this is set to true
	bool visible;
	/// @brief: A request to refresh this object. When this is set, the object will be completely
	/// redrawn on the next step cycle.
	/// @note: It depends on the object if this feature is implemented or not.
	bool refresh;
	/// @brief: Normally true. If the object has a enabled/disable functionality,
	/// this flag is used to determine if the object is enabled. Usually disabled
	/// objects appear with less saturation
	bool enabled;
	/// @brief: Holds a pointer to a transition attached to this object (if any)
	uv_uitransition_st *transition;
} uv_uiobject_st;



/// @brief: Initializes the bounding box
void uv_bounding_box_init(uv_bounding_box_st *bb,
		int16_t x, int16_t y, uint16_t width, uint16_t height);



#define this ((uv_uiobject_st*) me)


/// @brief: Sets the refresh request to this object. If the object's
/// dimensions doesn't change, this is enough to update the screen.
///
/// @param this: Pointer to uv_uiobject_st casted to void*.
#if CONFIG_LCD
static inline void uv_ui_refresh(void *me) {
	this->refresh = true;
}

#elif CONFIG_FT81X
void uv_ui_refresh(void *me);
#endif


/// @brief: Refreshes the object's parent. With this it is guaranteed that
/// everything gets refreshed the right way, but this has more overheat than uv_ui_refresh.
void uv_ui_refresh_parent(void *me);


/// @brief: Initializes an object
void uv_uiobject_init(void *me);


/// @brief: uiobject step function
uv_uiobject_ret_e uv_uiobject_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb);

/// @brief: Set's the drawing function
static inline void uv_uiobject_set_draw_callb(void *me,
		void (*vrtl_draw)(void *, const uv_bounding_box_st *)) {
	((uv_uiobject_st*) me)->vrtl_draw = vrtl_draw;
}


/// @brief: Calls the redraw callback function if one is assigned
void _uv_uiobject_draw(void *me, const uv_bounding_box_st *pbb);

/// @brief: Sets the virtual touch function pointer
static inline void uv_uiobject_set_touch_callb(void *me,
		void (*vrtl_touch)(void *, uv_touch_st *)) {
	((uv_uiobject_st*) me)->vrtl_touch = vrtl_touch;
}

/// @brief: Sets the step callback function
static inline void uv_uiobject_set_step_callb(void *me,
		uv_uiobject_ret_e (*step_callb)(void *, uint16_t, const uv_bounding_box_st *)) {
	((uv_uiobject_st*) me)->step_callb = step_callb;
}

/// @brief: Hides the object form the display
///
/// @param this: Pointer to uv_uiobject_st casted to void*.
void uv_ui_hide(void *me);


/// @brief: Shows the object
///
/// @param this: Pointer to uv_uiobject_st casted to void*.
static inline void uv_ui_show(void *me) {
	this->visible = true;
	uv_ui_refresh_parent(this);
}

/// @brief: Enabled the object. This functionality might not be implemented
/// on all obejcts.
void uv_ui_set_enabled(void *me, bool enabled);
static inline void uv_uiobject_enable(void *me) {
	uv_ui_set_enabled(me, true);
}
static inline void uv_uiobject_disable(void *me) {
	uv_ui_set_enabled(me, false);
}
static inline void uv_uiobject_set_enabled(void *me, bool value) {
	uv_ui_set_enabled(me, value);
}

static inline bool uv_ui_get_enabled(const void *me) {
	return this->enabled;
}

/// @brief: Getter for the object's bounding box
///
/// @param this: Pointer to uv_uiobject_st casted to void*.
static inline uv_bounding_box_st* uv_ui_get_bb(const void *me) {
	return &this->bb;
}
static inline uv_bounding_box_st *uv_uibb(const void *me) {
	return &this->bb;
}


static inline void uv_ui_add_transition(void *me, void *transition) {
	this->transition = transition;
}


/// @brief: Returns the X coordinate as global
///
/// @param this: Pointer to uv_uiobject_st casted to void*.
int16_t uv_ui_get_xglobal(const void *me);


/// @brief: Returns the Y coordinate as global
///
/// @param this: Pointer to uv_uiobject_st casted to void*.
int16_t uv_ui_get_yglobal(const void *me);



#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_OBJECT_H_ */
