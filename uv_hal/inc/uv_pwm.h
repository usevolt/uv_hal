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
#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#elif CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#endif


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

#if CONFIG_TARGET_LPC15XX


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
#define PWM0_0		1
#endif
#if CONFIG_PWM0_1
#if !defined(CONFIG_PWM0_1_IO)
#error "CONFIG_PWM0_1_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM0_1		2
#endif
#if CONFIG_PWM0_2
#if !defined(CONFIG_PWM0_2_IO)
#error "CONFIG_PWM0_2_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM0_2		3
#endif
#if CONFIG_PWM0_3
#if CONFIG_PWM0_3_IO
#error "PWM0_3 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_3_IO undefined or set to 0"
#endif
#define PWM0_3		4
#endif
#if CONFIG_PWM0_4
#if CONFIG_PWM0_4_IO
#error "PWM0_4 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_4_IO undefined or set to 0"
#endif
#define PWM0_4		5
#endif
#if CONFIG_PWM0_5
#if CONFIG_PWM0_5_IO
#error "PWM0_5 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_5_IO undefined or set to 0"
#endif
#define PWM0_5		6
#endif
#if CONFIG_PWM0_6
#if CONFIG_PWM0_6_IO
#error "PWM0_6 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_6_IO undefined or set to 0"
#endif
#define PWM0_6		7
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
#define PWM1_0		9
#endif
#if CONFIG_PWM1_1
#if !defined(CONFIG_PWM1_1_IO)
#error "CONFIG_PWM1_1_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM1_1		10
#endif
#if CONFIG_PWM1_2
#if !defined(CONFIG_PWM1_2_IO)
#error "CONFIG_PWM1_2_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM1_2		11
#endif
#if CONFIG_PWM1_3
#if CONFIG_PWM1_3_IO
#error "PWM1_3 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_3_IO undefined or set to 0"
#endif
#define PWM1_3		12
#endif
#if CONFIG_PWM1_4
#if CONFIG_PWM1_4_IO
#error "PWM1_4 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_4_IO undefined or set to 0"
#endif
#define PWM1_4		13
#endif
#if CONFIG_PWM1_5
#if CONFIG_PWM1_5_IO
#error "PWM1_5 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_5_IO undefined or set to 0"
#endif
#define PWM1_5		14
#endif
#if CONFIG_PWM1_6
#if CONFIG_PWM1_6_IO
#error "PWM1_6 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_6_IO undefined or set to 0"
#endif
#define PWM1_6		15
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
#define PWM2_0		17
#endif
#if CONFIG_PWM2_1
#if !defined(CONFIG_PWM2_1_IO)
#error "CONFIG_PWM2_1_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM2_1		18
#endif
#if CONFIG_PWM2_2
#if !defined(CONFIG_PWM2_2_IO)
#error "CONFIG_PWM2_2_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM2_2		19
#endif
#if CONFIG_PWM2_3
#if CONFIG_PWM2_3_IO
#error "PWM2_3 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM2_3_IO undefined or set to 0"
#endif
#define PWM2_3		20
#endif
#if CONFIG_PWM2_4
#if CONFIG_PWM2_4_IO
#error "PWM2_4 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM2_4_IO undefined or set to 0"
#endif
#define PWM2_4		21
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
#define PWM3_0		25
#endif
#if CONFIG_PWM3_1
#if !defined(CONFIG_PWM3_1_IO)
#error "CONFIG_PWM3_1_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM3_1		26
#endif
#if CONFIG_PWM3_2
#if !defined(CONFIG_PWM3_2_IO)
#error "CONFIG_PWM3_2_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM3_2		27
#endif
#if CONFIG_PWM3_3
#if CONFIG_PWM3_3_IO
#error "PWM3_3 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM3_3_IO undefined or set to 0"
#endif
#define PWM3_3		28
#endif
#if CONFIG_PWM3_4
#if CONFIG_PWM3_4_IO
#error "PWM3_4 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM3_4_IO undefined or set to 0"
#endif
#define PWM3_4		29
#endif
#endif

/// @brief: Returns the module number of pwm channel.
#define PWM_GET_MODULE(pwm)		(((pwm) - 1) / 8)
/// @brief: Returns the channel number from the module (0-7).
#define PWM_GET_CHANNEL(pwm)	(((pwm) - 1) % 8)


#elif CONFIG_TARGET_LPC1785
#if !defined(CONFIG_PWM0_1) && \
!defined(CONFIG_PWM0_2) && \
!defined(CONFIG_PWM0_3) && \
!defined(CONFIG_PWM0_4) && \
!defined(CONFIG_PWM0_5) && \
!defined(CONFIG_PWM0_6) && \
!defined(CONFIG_PWM1_1) && \
!defined(CONFIG_PWM1_2) && \
!defined(CONFIG_PWM1_3) && \
!defined(CONFIG_PWM1_4) && \
!defined(CONFIG_PWM1_5) && \
!defined(CONFIG_PWM1_6)
#error "Enable at least 1 PWM output by defining CONFIG_PWMx_CHx macros"
#endif

// shortcut macros for checking if the PWM is enabled
#if (CONFIG_PWM0_1 || \
		CONFIG_PWM0_2 || \
		CONFIG_PWM0_3 || \
		CONFIG_PWM0_4 || \
		CONFIG_PWM0_5 || \
		CONFIG_PWM0_6)
#define CONFIG_PWM0			1
#endif
// shortcut macros for checking if the PWM is enabled
#if (CONFIG_PWM1_1 || \
		CONFIG_PWM1_2 || \
		CONFIG_PWM1_3 || \
		CONFIG_PWM1_4 || \
		CONFIG_PWM1_5 || \
		CONFIG_PWM1_6)
#define CONFIG_PWM1			1
#endif


#if CONFIG_PWM0_1
#define PWM0_1				&LPC_PWM0->MR1
#if !defined(CONFIG_PWM0_1_IO)
#error "CONFIG_PWM0_1_IO shoud define the IO_Config register of the pin where PWM0_1 is output.\
 Note that only pins which can be used to output PWM0_1 should be defined."
#endif
#endif
#if CONFIG_PWM0_2
#define PWM0_2				&LPC_PWM0->MR2
#if !defined(CONFIG_PWM0_2_IO_CONF)
#error "CONFIG_PWM0_2_IO_CONF should define the LPC_IOCON->xxx configuration to enable PWM output on specified pin."
#endif
#endif
#if CONFIG_PWM0_3
#define PWM0_3				&LPC_PWM0->MR3
#if !defined(CONFIG_PWM0_1_IO_CONF)
#error "CONFIG_PWM0_2_IO_CONF should define the LPC_IOCON->xxx configuration to enable PWM output on specified pin."
#endif
#endif
#if CONFIG_PWM0_4
#define PWM0_4				&LPC_PWM0->MR4
#if !defined(CONFIG_PWM0_1_IO_CONF)
#error "CONFIG_PWM0_2_IO_CONF should define the LPC_IOCON->xxx configuration to enable PWM output on specified pin."
#endif
#endif
#if CONFIG_PWM0_5
#define PWM0_5				&LPC_PWM0->MR5
#if !defined(CONFIG_PWM0_1_IO_CONF)
#error "CONFIG_PWM0_2_IO_CONF should define the LPC_IOCON->xxx configuration to enable PWM output on specified pin."
#endif
#endif
#if CONFIG_PWM0_6
#define PWM0_6				&LPC_PWM0->MR6
#if !defined(CONFIG_PWM0_1_IO_CONF)
#error "CONFIG_PWM0_2_IO_CONF should define the LPC_IOCON->xxx configuration to enable PWM output on specified pin."
#endif
#endif
#if CONFIG_PWM1_1
#define PWM1_1				&LPC_PWM1->MR1
#if !defined(CONFIG_PWM1_1_IO_CONF)
#error "CONFIG_PWM0_2_IO_CONF should define the LPC_IOCON->xxx configuration to enable PWM output on specified pin."
#endif
#endif
#if CONFIG_PWM1_2
#define PWM1_2				&LPC_PWM1->MR2
#if !defined(CONFIG_PWM1_2_IO_CONF)
#error "CONFIG_PWM0_2_IO_CONF should define the LPC_IOCON->xxx configuration to enable PWM output on specified pin."
#endif
#endif
#if CONFIG_PWM1_3
#define PWM1_3				&LPC_PWM1->MR3
#if !defined(CONFIG_PWM1_3_IO_CONF)
#error "CONFIG_PWM0_2_IO_CONF should define the LPC_IOCON->xxx configuration to enable PWM output on specified pin."
#endif
#endif
#if CONFIG_PWM1_4
#define PWM1_4				&LPC_PWM1->MR4
#if !defined(CONFIG_PWM1_4_IO_CONF)
#error "CONFIG_PWM0_2_IO_CONF should define the LPC_IOCON->xxx configuration to enable PWM output on specified pin."
#endif
#endif
#if CONFIG_PWM1_5
#define PWM1_5				&LPC_PWM1->MR5
#if !defined(CONFIG_PWM1_5_IO_CONF)
#error "CONFIG_PWM0_2_IO_CONF should define the LPC_IOCON->xxx configuration to enable PWM output on specified pin."
#endif
#endif
#if CONFIG_PWM1_6
#define PWM1_6				&LPC_PWM1->MR6
#if !defined(CONFIG_PWM1_6_IO_CONF)
#error "CONFIG_PWM0_2_IO_CONF should define the LPC_IOCON->xxx configuration to enable PWM output on specified pin."
#endif
#endif

#elif CONFIG_TARGET_LPC11C14

#if !defined(CONFIG_PWM0_1) && \
!defined(CONFIG_PWM0_2) && \
!defined(CONFIG_PWM0_3) && \
!defined(CONFIG_PWM1_1) && \
!defined(CONFIG_PWM1_2) && \
!defined(CONFIG_PWM2_1) && \
!defined(CONFIG_PWM2_2) && \
!defined(CONFIG_PWM2_3) && \
!defined(CONFIG_PWM3_1) && \
!defined(CONFIG_PWM3_2) && \
!defined(CONFIG_PWM3_3)
#error "Enable at least 1 PWM output by defining CONFIG_PWMx_CHx macros"
#endif


#if CONFIG_PWM0_1
#define PWM0_1				&LPC_TMR16B0->MR0
#endif

#if CONFIG_PWM0_2
#define PWM0_2				&LPC_TMR16B0->MR1
#endif

#if CONFIG_PWM0_3
#define PWM0_3				&LPC_TMR16B0->MR2
#endif

#if CONFIG_PWM1_1
#define PWM1_1				&LPC_TMR16B1->MR0
#endif

#if CONFIG_PWM1_2
#define PWM1_2				&LPC_TMR16B1->MR1
#endif

#if CONFIG_PWM2_1
#define PWM2_1				&LPC_TMR32B0->MR0
#endif

#if CONFIG_PWM2_2
#define PWM2_2				&LPC_TMR32B0->MR1
#endif

#if CONFIG_PWM2_3
#define PWM2_3				&LPC_TMR32B0->MR3
#endif

#if CONFIG_PWM3_1
#define PWM3_1				&LPC_TMR32B1->MR0
#endif

#if CONFIG_PWM3_2
#define PWM3_2				&LPC_TMR32B1->MR1
#endif

#if CONFIG_PWM3_3
#define PWM3_3				&LPC_TMR32B1->MR2
#endif

// shortcut macros for checking if the PWM is enabled
#if (CONFIG_PWM0_1 || \
		CONFIG_PWM0_2 || \
		CONFIG_PWM0_3)
#define CONFIG_PWM0			1
#endif
#if (CONFIG_PWM1_1 || \
		CONFIG_PWM1_2)
#define CONFIG_PWM1			1
#endif
#if (CONFIG_PWM2_1 || \
		CONFIG_PWM2_2 || \
		CONFIG_PWM2_3)
#define CONFIG_PWM2			1
#endif
#if (CONFIG_PWM3_1 || \
		CONFIG_PWM3_2 || \
		CONFIG_PWM3_3)
#define CONFIG_PWM3			1
#endif

#endif

/// @brief: Variable to separate different PWM channels from each other
/// Possible values are PWM channel macros defined upper.
typedef volatile uint32_t uv_pwm_channel_t;


#define PWM_MAX_VALUE		1000U

/// @brief: Can be used to adjust the PWM duty cycle regardless of the MAX value
#define DUTY_CYCLE(x_float)	(uint16_t)((float) PWM_MAX_VALUE * ((float) x_float))

#define DUTY_CYCLEPPT(ppt)	((uint32_t) PWM_MAX_VALUE * (ppt) / 1000)

/// @brief: This device as PWM module, if modules are deifned with uv_pwm_init_module
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
