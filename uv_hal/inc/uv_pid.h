/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
	uint8_t p;
	/// @brief: I factor
	uint8_t i;
	/// @brief: D factor
	uint8_t d;
	int16_t sum;
	int16_t input;
	int16_t output;
	pid_state_e state;
} uv_pid_st;


/// @brief: Initializes the PID structure
void uv_pid_init(uv_pid_st *this, uint8_t p, uint8_t i, uint8_t d);

/// @brief: PID step function
void uv_pid_step(uv_pid_st *this, uint16_t step_ms, int16_t input);

/// @brief: Returns the output from the PID
static inline int16_t uv_pid_get_output(uv_pid_st *this) {
	return this->output;
}

/// @brief: Resets the PID state to zero
static inline void uv_pid_reset(uv_pid_st *this) {
	uv_pid_init(this, this->p, this->i, this->d);
}

/// @brief: Sets the P factor. Valid range is from 0 to 255.
static inline void uv_pid_set_p(uv_pid_st *this, uint8_t p) {
	this->p = p;
}

/// @brief: Returns the P factor. Valid range is from 0 to 255.
static inline uint8_t uv_pid_get_p(uv_pid_st *this) {
	return this->p;
}

/// @brief: Sets the I factor. Valid range is from 0 to 255.
static inline void uv_pid_set_i(uv_pid_st *this, uint8_t i) {
	this->i = i;
}

/// @brief: Return the I factor. Valid range is from 0 to 255.
static inline uint8_t uv_pid_get_i(uv_pid_st *this) {
	return this->i;
}

/// @brief: Sets the D factor. Valid range is from 0 to 255.
static inline void uv_pid_set_d(uv_pid_st *this, uint8_t d) {
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
