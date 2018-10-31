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


#ifndef UW_FILTERS_H_
#define UW_FILTERS_H_


#include "uv_hal_config.h"
#include <uv_utilities.h>


typedef struct {
	int32_t sum;
	int32_t count;
	int32_t cur_count;
	int32_t val;
} uv_moving_aver_st;

/// @brief: Initializes the moving average filter
void uv_moving_aver_init (uv_moving_aver_st *avr, int32_t cnt);

/// @brief: Resets the moving average filter to zero
void uv_moving_aver_reset (uv_moving_aver_st *avr);

/// @brief: Moving average step function. Should be called with a constant step time
int32_t  uv_moving_aver_step (uv_moving_aver_st *avr, int32_t val);

/// @brief: Returns the current moving average value
static inline int32_t uv_moving_aver_get_val(uv_moving_aver_st *this) {
	return (this->val / 0x100);
}

/// @brief: Sets the moving average filter count
void uv_moving_aver_set_count(uv_moving_aver_st *this, int32_t value);

/// @brief: Returns true when the moving average buffer is full, e.g. when
/// the step function has been called enough times
static inline bool uv_moving_aver_is_full(uv_moving_aver_st *this) {
	return (this->count == this->cur_count);
}


typedef struct {
	int32_t trigger_value;
	int32_t hysteresis;
	uint8_t result;
	bool invert;
} uv_hysteresis_st;

/// @brief: Initializes the hysteresis filter
///
/// @param trigger_value: Value at where toggling the output is done
/// @param Hysteresis: The hysteresis around trigger_value
/// @param invert: If false, output is 0 when the value is smaller than trigger_value.
void uv_hysteresis_init(uv_hysteresis_st *this, int32_t trigger_value, int32_t hysteresis, bool invert);

bool uv_hysteresis_step(uv_hysteresis_st *this, int32_t value);

/// @brief: Sets the trigger value
///
/// @note: Glitching or flickering or output might happen from unnecessary changes
static inline void uv_hysteresis_set_trigger_value(uv_hysteresis_st *this, int32_t value) {
	this->trigger_value = value;
}

static inline uint8_t uv_hysteresis_get_output(uv_hysteresis_st *this) {
	return this->result;
}


#endif /* UW_FILTERS_H_ */
