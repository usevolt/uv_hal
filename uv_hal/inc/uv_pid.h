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


#ifndef UV_HAL_INC_UV_PID_H_
#define UV_HAL_INC_UV_PID_H_

#include <uv_hal_config.h>
#include <uv_utilities.h>

/// @file: UV implementation of a PID controller



#if CONFIG_PID


#define PID_OFF_REQ_TOLERANCE	5

/// @brief: PID states. Will be used internally.
typedef enum {
	PID_STATE_OFF,
	PID_STATE_ON,
	PID_STATE_OFF_REQ
} pid_state_e;


/// @brief: Main PID controller structure
typedef struct {
	/// @brief: P factor
	uint32_t p;
	/// @brief: I factor
	uint32_t i;
	/// @brief: D factor
	uint32_t d;
	int32_t sum;
	int32_t max_sum;
	int32_t min_sum;
	int32_t input;
	int32_t target;
	int32_t output;
	int32_t last_err;
	pid_state_e state;
} uv_pid_st;


/// @brief: Initializes the PID structure
void uv_pid_init(uv_pid_st *this, uint32_t p, uint32_t i, uint32_t d);

/// @brief: PID step function
void uv_pid_step(uv_pid_st *this, uint16_t step_ms, int32_t input);

/// @brief: Returns the output from the PID
static inline int32_t uv_pid_get_output(uv_pid_st *this) {
	return this->output;
}

/// @brief: Sets the PID controller target value
static inline void uv_pid_set_target(uv_pid_st *this, int32_t value) {
	this->target = value;
}

/// @brief: Resets the PID state to zero
static inline void uv_pid_reset(uv_pid_st *this) {
	uv_pid_init(this, this->p, this->i, this->d);
}

/// @brief: Sets the P factor. Valid range is from 0 to 65535.
static inline void uv_pid_set_p(uv_pid_st *this, uint32_t p) {
	this->p = p;
}

/// @brief: Returns the P factor. Valid range is from 0 to 65535.
static inline uint8_t uv_pid_get_p(uv_pid_st *this) {
	return this->p;
}

/// @brief: Sets the I factor. Valid range is from 0 to 65535.
static inline void uv_pid_set_i(uv_pid_st *this, uint32_t i) {
	this->i = i;
}

/// @brief: Sets the maximum sum value. Defaults to INT16_MAX.
static inline void uv_pid_set_max_sum(uv_pid_st *this, int32_t value) {
	this->max_sum = value;
}

/// @brief: Sets the minimum sum value. Defaults to INT16_MIN.
static inline void uv_pid_set_min_sum(uv_pid_st *this, int32_t value) {
	this->min_sum = value;
}

/// @brief: Return the I factor. Valid range is from 0 to 65535.
static inline uint8_t uv_pid_get_i(uv_pid_st *this) {
	return this->i;
}

/// @brief: Sets the D factor. Valid range is from 0 to 65535.
static inline void uv_pid_set_d(uv_pid_st *this, uint32_t d) {
	this->d = d;
}

/// @brief: Returns the D factor. Valid range is from 0 to 255.
static inline uint8_t uv_pid_get_d(uv_pid_st *this) {
	return this->d;
}

/// @brief: Enables the PID. PID is initially enabled.
static inline void uv_pid_enable(uv_pid_st *this) {
	this->state = PID_STATE_ON;
}

/// @brief: Disables the PID. When disabling, PID drives itself to zero,
/// then remains there.
static inline void uv_pid_disable(uv_pid_st *this) {
	this->state = PID_STATE_OFF_REQ;
}



#endif

#endif /* UV_HAL_INC_UV_PID_H_ */
