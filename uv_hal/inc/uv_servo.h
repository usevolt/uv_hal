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
