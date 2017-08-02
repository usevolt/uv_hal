/*
 * uv_uitransition.h
 *
 *  Created on: Aug 1, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UITRANSITION_H_
#define UV_HAL_INC_UI_UV_UITRANSITION_H_



/// @file: Defines different uitransitions which can be used to animate
/// UI objects. Note that by default UI objects are static and animating
/// some parameters from the might cause unexpected results.


#include "uv_ui.h"


#if CONFIG_UI


/// @brief: Defines all different states for the uitransitions
typedef enum {
	UITRANSITION_INIT = 0,
	UITRANSITION_PLAY,
	UITRANSITION_PAUSE,
	UITRANSITION_FINISH
} uv_uitransition_state_e;

/// @brief: uitransition easing values
typedef enum {
	UITRANSITION_EASING_LINEAR = 0,
	UITRANSITION_EASING_OUT_QUAD = (1 << 0),
	UITRANSITION_EASING_IN_QUAD = (1 << 1),
	UITRANSITION_EASING_INOUT_QUAD = UITRANSITION_EASING_OUT_QUAD |
								UITRANSITION_EASING_IN_QUAD
} uv_uitransition_easing_e;


/// @brief: Main uitransition pure virtual structure.
struct _uv_uitransition_st {

	uv_uitransition_state_e state;
	uv_uitransition_st *parallel;
	uv_uitransition_st *series;
	uv_uitransition_easing_e easing;
	uint16_t duration_ms;
	uint16_t current_time_ms;
	uint16_t speed_ppt;
	/// @brief: Virtual function which should implement
	/// the calculations every step cycle
	void (*calc_callb)(void *me);
};


#if defined(this)
#undef this
#endif
#define this ((uv_uitransition_st*) me)

/// @brief: Initializes the basic transition object
void uv_uitransition_init(void *me, uv_uitransition_easing_e easing,
		uint16_t duration_ms, void (*calc_callb)(void *me));


/// @brief: Starts the uitransition
void uv_uitransition_start(void *me);


/// @brief: Pauses the uitransition
void uv_uitransition_pause(void *me);


/// @brief: Adds a parallel transition
static inline void uv_uitransition_add_parallel(void *me,
		void *parallel_transition) {
	this->parallel = parallel_transition;
}

/// @brief: Adds a serial transition
static inline void uv_uitransition_add_series(void *me,
		void *series_transition) {
	this->series = series_transition;
}


/// @brief: Returns true if the transition is finished
static inline bool uv_uitransition_is_finished(void *me) {
	return (this->state == UITRANSITION_FINISH);
}

static inline uv_uitransition_state_e uv_uitransition_get_state(const void *me) {
	return this->state;
}

/// @brief: Sets the animation speed as a ppt value. Defaults to 1000.
static inline void uv_uitransition_set_speed(void *me, uint16_t speed_ppt) {
	this->speed_ppt = speed_ppt;
}

/// @bief: Step function is called automatically from the uiobject where
/// the transition is attached.
///
/// @param parent: The parent object to which this uitransition is attached
void uv_uitransition_step(void *me, uv_uiobject_st *parent, uint16_t step_ms);


/// @bief: Transition animating a signed 16 bit integer value.
/// Since 16-bit integers are used in most places in UI library.
typedef struct {
	EXTENDS(uv_uitransition_st);

	int16_t start_val;
	int16_t end_val;
	int16_t *cur_val;
} uv_uiscalartransition_st;


/// @brief: Initializes the scalar transition
void uv_uiscalartransition_init(void *me, uv_uitransition_easing_e easing,
		uint16_t duration_ms, int16_t start_val, int16_t end_val, int16_t *val_ptr);



/// @brief: Transition animating a color value
typedef struct {
	EXTENDS(uv_uitransition_st);

	color_t start_c;
	color_t end_c;
	color_t *cur_c;
} uv_uicolortransition_st;


/// @brief: Initializes a color transition
void uv_uicolortransition_init(void *me, uv_uitransition_easing_e easing,
		uint16_t duration_ms, color_t start_c, color_t end_c, color_t *c_ptr);



#undef this


#endif

#endif /* UV_HAL_INC_UI_UV_UITRANSITION_H_ */
