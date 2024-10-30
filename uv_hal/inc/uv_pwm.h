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

#ifndef UV_HAL_INC_UV_PWM_H_
#define UV_HAL_INC_UV_PWM_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"


#if CONFIG_PWM


#if CONFIG_TARGET_LPC15XX
#define PWM_CHN(_pwm_chn)	_pwm_chn
#else
// PWM channels are disabled for Linux & Win targets
// This is defined as 1 instead of 0 since 0 can mean an undefined or uninitialized
// pwm channel
#define PWM_CHN(_pwm_chn)	1
#endif
#if !defined(CONFIG_PWMEXT_MODULE_COUNT)
#define CONFIG_PWMEXT_MODULE_COUNT			0
#endif


enum {
	PWM0_0 = 1,
	PWM0_1,
	PWM0_2,
	PWM0_3,
	PWM0_4,
	PWM0_5,
	PWM0_6,
	PWM0_7,
	PWM1_0,
	PWM1_1,
	PWM1_2,
	PWM1_3,
	PWM1_4,
	PWM1_5,
	PWM1_6,
	PWM1_7,
	PWM2_0,
	PWM2_1,
	PWM2_2,
	PWM2_3,
	PWM2_4,
	PWM2_5,
	PWM2_6,
	PWM2_7,
	PWM3_0,
	PWM3_1,
	PWM3_2,
	PWM3_3,
	PWM3_4,
	PWM3_5,
	PWM3_6,
	PWM3_7
};


#if (!defined(CONFIG_PWM0) && !defined(CONFIG_PWM1) &&  \
		!defined(CONFIG_PWM2) && !defined(CONFIG_PWM3))
#error "Either PWM0, PWM1, PWM2 or PWM3 module should be enabled with CONFIG_PWMx symbol."
#endif
#if CONFIG_PWM0
#if !defined(CONFIG_PWM0_FREQ)
#error "CONFIG_PWM0_FREQ should define the PWM frequency in Hz"
#endif
#if CONFIG_PWM0_0
#if !defined(CONFIG_PWM0_0_IO)
#error "CONFIG_PWM0_0_IO should define the GPIO pin used as the PWM output"
#endif
#endif
#if CONFIG_PWM0_1
#if !defined(CONFIG_PWM0_1_IO)
#error "CONFIG_PWM0_1_IO should define the GPIO pin used as the PWM output"
#endif
#endif
#if CONFIG_PWM0_2
#if !defined(CONFIG_PWM0_2_IO)
#error "CONFIG_PWM0_2_IO should define the GPIO pin used as the PWM output"
#endif
#endif
#if CONFIG_PWM0_3
#if CONFIG_PWM0_3_IO
#error "PWM0_3 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_3_IO undefined or set to 0"
#endif
#endif
#if CONFIG_PWM0_4
#if CONFIG_PWM0_4_IO
#error "PWM0_4 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_4_IO undefined or set to 0"
#endif
#endif
#if CONFIG_PWM0_5
#if CONFIG_PWM0_5_IO
#error "PWM0_5 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_5_IO undefined or set to 0"
#endif
#endif
#if CONFIG_PWM0_6
#if CONFIG_PWM0_6_IO
#error "PWM0_6 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_6_IO undefined or set to 0"
#endif
#endif
#endif
#if CONFIG_PWM1
#if !defined(CONFIG_PWM1_FREQ)
#error "CONFIG_PWM1_FREQ should define the PWM frequency in Hz"
#endif
#if CONFIG_PWM1_0
#if !defined(CONFIG_PWM1_0_IO)
#error "CONFIG_PWM1_0_IO should define the GPIO pin used as the PWM output"
#endif
#endif
#if CONFIG_PWM1_1
#if !defined(CONFIG_PWM1_1_IO)
#error "CONFIG_PWM1_1_IO should define the GPIO pin used as the PWM output"
#endif
#endif
#if CONFIG_PWM1_2
#if !defined(CONFIG_PWM1_2_IO)
#error "CONFIG_PWM1_2_IO should define the GPIO pin used as the PWM output"
#endif
#endif
#if CONFIG_PWM1_3
#if CONFIG_PWM1_3_IO
#error "PWM1_3 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_3_IO undefined or set to 0"
#endif
#endif
#if CONFIG_PWM1_4
#if CONFIG_PWM1_4_IO
#error "PWM1_4 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_4_IO undefined or set to 0"
#endif
#endif
#if CONFIG_PWM1_5
#if CONFIG_PWM1_5_IO
#error "PWM1_5 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_5_IO undefined or set to 0"
#endif
#endif
#if CONFIG_PWM1_6
#if CONFIG_PWM1_6_IO
#error "PWM1_6 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_6_IO undefined or set to 0"
#endif
#endif
#endif
#if CONFIG_PWM2
#if !defined(CONFIG_PWM2_FREQ)
#error "CONFIG_PWM2_FREQ should define the PWM frequency in Hz"
#endif
#if CONFIG_PWM2_0
#if !defined(CONFIG_PWM2_0_IO)
#error "CONFIG_PWM2_0_IO should define the GPIO pin used as the PWM output"
#endif
#endif
#if CONFIG_PWM2_1
#if !defined(CONFIG_PWM2_1_IO)
#error "CONFIG_PWM2_1_IO should define the GPIO pin used as the PWM output"
#endif
#endif
#if CONFIG_PWM2_2
#if !defined(CONFIG_PWM2_2_IO)
#error "CONFIG_PWM2_2_IO should define the GPIO pin used as the PWM output"
#endif
#endif
#if CONFIG_PWM2_3
#if CONFIG_PWM2_3_IO
#error "PWM2_3 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM2_3_IO undefined or set to 0"
#endif
#endif
#if CONFIG_PWM2_4
#if CONFIG_PWM2_4_IO
#error "PWM2_4 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM2_4_IO undefined or set to 0"
#endif
#endif
#endif
#if CONFIG_PWM3
#if !defined(CONFIG_PWM3_FREQ)
#error "CONFIG_PWM3_FREQ should define the PWM frequency in Hz"
#endif
#if CONFIG_PWM3_0
#if !defined(CONFIG_PWM3_0_IO)
#error "CONFIG_PWM3_0_IO should define the GPIO pin used as the PWM output"
#endif
#endif
#if CONFIG_PWM3_1
#if !defined(CONFIG_PWM3_1_IO)
#error "CONFIG_PWM3_1_IO should define the GPIO pin used as the PWM output"
#endif
#endif
#if CONFIG_PWM3_2
#if !defined(CONFIG_PWM3_2_IO)
#error "CONFIG_PWM3_2_IO should define the GPIO pin used as the PWM output"
#endif
#endif
#if CONFIG_PWM3_3
#if CONFIG_PWM3_3_IO
#error "PWM3_3 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM3_3_IO undefined or set to 0"
#endif
#endif
#if CONFIG_PWM3_4
#if CONFIG_PWM3_4_IO
#error "PWM3_4 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM3_4_IO undefined or set to 0"
#endif
#endif
#endif

/// @brief: Returns the module number of pwm channel.
#define PWM_GET_MODULE(pwm)		(((pwm) - 1) / 8)
/// @brief: Returns the channel number from the module (0-7).
#define PWM_GET_CHANNEL(pwm)	(((pwm) - 1) % 8)


/// @brief: Variable to separate different PWM channels from each other
/// Possible values are PWM channel macros defined upper.
typedef volatile uint32_t uv_pwm_channel_t;


#define PWM_MAX_VALUE		1000U

/// @brief: Can be used to adjust the PWM duty cycle regardless of the MAX value
#define DUTY_CYCLE(x_float)	(uint16_t)((float) PWM_MAX_VALUE * ((float) x_float))

#define DUTY_CYCLEPPT(ppt)	((uint32_t) PWM_MAX_VALUE * (ppt) / 1000)

/// @brief: This device as PWM module, if modules are defined with uv_pwm_init_module
#define PWMEXT_MODULE_THIS			0

#define PWMEXT_GET_MODULE(chn)		((chn) >> 24)

#define PWMEXT_GET_CHN(chn)		((chn) & ~(0xFF << 24))

#define PWMEXT_CHN(module, chn)	(((module) << 24) | (chn))


/// @brief: Initializes the PWM modules
uv_errors_e _uv_pwm_init();



/// @brief: Sets the PWM channels output.
///
/// @param chn: The PWM channel to be set
/// @param value: The PWM value given in the range of 0 ... PWM_MAX_VALUE
uv_errors_e uv_pwm_set(uv_pwm_channel_t chn, uint16_t value);



/// @brief: Sets the PWM frequency. Note that all pwm channels from the same pwm module
/// share the same frequency. (PWM0_0, PWM0_1, PWM0_2, etc)
void uv_pwm_set_freq(uv_pwm_channel_t chn, uint32_t value);



/// @brief: Returns the current PWM value
uint16_t uv_pwm_get(uv_pwm_channel_t chn);



/// @brief: Initializes an external PWM module. This allows the PWM module to control
/// external PWM'sother than the device's own channels. The external PWM channels are
/// selected by using the PWMEXT_CHN() macro with the *module_index* in the module parameter.
///
/// @param module_index: Index of this module that is used pointing to this module when
/// selecting the PWM channel with PWMEXT_CHN macro. Has to be <= CONFIG_PWMEXT_MODULE_COUNT.
/// NOTE: The indexing starts from 1, since 0 equals to local PWM outputs.
///
/// @return: ERR_HARDWARE_NOT_SUPPORTED if the module_index is out of bounds
uv_errors_e uv_pwmext_module_init(
		uint8_t module_index,
		void *module_ptr,
		void (*set_callb)(void *module_ptr, uint32_t chn, uint16_t value),
		uint16_t (*get_callb)(void *module_ptr, uint32_t chn),
		void (*freq_callb)(void *module_ptr, uint32_t chn, uint32_t freq));

#endif

#endif /* UV_HAL_INC_UV_PWM_H_ */
