/*
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 *
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
	UITRANSITION_REVERSEPLAY,
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
typedef struct {

	void *parallel;
	void *series;
	int16_t duration_ms;
	int16_t current_time_ms;
	uv_uitransition_state_e state;
	uv_uitransition_easing_e easing;
	uint16_t speed_ppt;
	/// @brief: Virtual function which should implement
	/// the calculations every step cycle
	void (*calc_callb)(void *me);
	/// @brief: Callback which will be called when the transition state changes.
	/// This can be used to create different kind of looping transitions
	void (*state_change_callback)(void *me, uv_uitransition_state_e last_state);
} _uv_uitransition_st;


#if defined(this)
#undef this
#endif
#define this ((_uv_uitransition_st*) me)



/// @brief: Starts the uitransition
void uv_uitransition_play(void *me);

/// @brief: Starts the uitransition in reverse direction
void uv_uitransition_reverseplay(void *me);


/// @brief: Pauses the uitransition
void uv_uitransition_pause(void *me);

/// @brief: Stops the transition and sets it in INIT state
void uv_uitransition_stop(void *me);


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


/// @brief: Adds a state-changed callback to the transition
static inline void uv_uitransition_set_state_change_callback(void *me,
		void (*callb)(void *, uv_uitransition_state_e)) {
	this->state_change_callback = callb;
}



/// @brief: Sets the absolute position of the transition. Note that this
/// affects only if the transition is playing /reverse playing.
void uv_uitransition_set_position(uv_uitransition_st *me, uint16_t value);

/// @brief: Returns true if the transition is finished
static inline bool uv_uitransition_is_finished(void *me) {
	return (this->state == UITRANSITION_FINISH);
}

/// @brief: Returns the current state of the transition
static inline uv_uitransition_state_e uv_uitransition_get_state(const void *me) {
	return this->state;
}

/// @brief: Returns true if the transition is playing in normal or reverse direction
static inline bool uv_uitransition_is_playing(const void *me) {
	return (this->state == UITRANSITION_PLAY ||
			this->state == UITRANSITION_REVERSEPLAY);
}


/// @brief: Sets the easing for the transition
static inline void uv_uitransition_set_easing(void *me, uv_uitransition_easing_e easing) {
	this->easing = easing;
}

/// @brief: Sets the animation speed as a ppt value. Defaults to 1000.
static inline void uv_uitransition_set_speed(void *me, uint16_t speed_ppt) {
	this->speed_ppt = speed_ppt;
}


static inline int16_t uv_uitransition_get_duration_ms(void *me) {
	return this->duration_ms;
}



/// @bief: Transition animating a signed 16 bit integer value.
/// Since 16-bit integers are used in most places in UI library.
typedef struct {
	EXTENDS(_uv_uitransition_st);

	int16_t start_val;
	int16_t end_val;
	int16_t *cur_val;
} uv_uiscalartransition_st;


/// @brief: Initializes the scalar transition
void uv_uiscalartransition_init(void *me, uv_uitransition_easing_e easing,
		uint16_t duration_ms, int16_t start_val, int16_t end_val, int16_t *val_ptr);



/// @brief: Transition animating a color value
typedef struct {
	EXTENDS(_uv_uitransition_st);

	color_t start_c;
	color_t end_c;
	color_t *cur_c;
} uv_uicolortransition_st;


/// @brief: Initializes a color transition
void uv_uicolortransition_init(void *me, uv_uitransition_easing_e easing,
		uint16_t duration_ms, color_t start_c, color_t end_c, color_t *c_ptr);






/// @brief: Initializes the basic transition object.
/// This function shouldn't be called by the user application
void _uv_uitransition_init(void *me, uv_uitransition_easing_e easing,
		uint16_t duration_ms, void (*calc_callb)(void *me));


/// @bief: Step function is called automatically from the uiobject where
/// the transition is attached. This function shouldn't be called by the user application.
///
/// @param parent: The parent object to which this uitransition is attached. Will be
/// refreshed automatically when transition plays. Can also be set to NULL.
void uv_uitransition_step(void *me, void *parent, uint16_t step_ms);


#undef this


#endif

#endif /* UV_HAL_INC_UI_UV_UITRANSITION_H_ */
