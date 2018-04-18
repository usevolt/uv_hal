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


#ifndef UV_HAL_INC_UV_SERVO_H_
#define UV_HAL_INC_UV_SERVO_H_


#include "uv_hal_config.h"
#include "uv_utilities.h"
#include "uv_pwm.h"


#if CONFIG_SERVO

/// @file: Defines a servo PWM macros. Servo PWM uses normal PWM modules,
/// this file describes the PWM settings.


/// @brief: Default servo PWM frequency
#define SERVO_PWM_FREQ		50



/// @brief: Default (90 degree) servo duty cycle with 1.5 ms pulse width
#define SERVO_POS_90_DC				((uint16_t) (PWM_MAX_VALUE * 0.075f))

/// @brief: Minimum position (0 degree) servo duty cycle with 1 ms pulse width
#define SERVO_POS_0_DC				((uint16_t) (PWM_MAX_VALUE * 0.050f))

/// @brief: Maximum position (180 degree) servo duty cycle with 2 ms pulse width
#define SERVO_POS_180_DC			((uint16_t) (PWM_MAX_VALUE * 0.100f))

#define SERVO_MAX_MANGLE			180000
#define SERVO_DEF_MANGLE			90000
#define SERVO_MIN_MANGLE			0

/// @brief: converts millidegrees into servo duty cycle
///
/// @param pos_mdeg: Should be between 0 and 180000
void uv_servo_set(uv_pwm_channel_t chn, uint32_t pos_mdeg);


#endif

#endif /* UV_HAL_INC_UV_SERVO_H_ */
