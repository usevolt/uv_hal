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

#if CONFIG_TARGET_LPC1549


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
#define PWM0_0		0
#endif
#if CONFIG_PWM0_1
#if !defined(CONFIG_PWM0_1_IO)
#error "CONFIG_PWM0_1_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM0_1		1
#endif
#if CONFIG_PWM0_2
#if !defined(CONFIG_PWM0_2_IO)
#error "CONFIG_PWM0_2_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM0_2		2
#endif
#if CONFIG_PWM0_3
#if CONFIG_PWM0_3_IO
#error "PWM0_3 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_3_IO undefined or set to 0"
#endif
#define PWM0_3		3
#endif
#if CONFIG_PWM0_4
#if CONFIG_PWM0_4_IO
#error "PWM0_4 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_4_IO undefined or set to 0"
#endif
#define PWM0_4		4
#endif
#if CONFIG_PWM0_5
#if CONFIG_PWM0_5_IO
#error "PWM0_5 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_5_IO undefined or set to 0"
#endif
#define PWM0_5		5
#endif
#if CONFIG_PWM0_6
#if CONFIG_PWM0_6_IO
#error "PWM0_6 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_6_IO undefined or set to 0"
#endif
#define PWM0_6		6
#endif
#if CONFIG_PWM0_7
#if CONFIG_PWM0_7_IO
#error "PWM0_7 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM0_7_IO undefined or set to 0"
#endif
#define PWM0_7		7
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
#define PWM1_0		8
#endif
#if CONFIG_PWM1_1
#if !defined(CONFIG_PWM1_1_IO)
#error "CONFIG_PWM1_1_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM1_1		9
#endif
#if CONFIG_PWM1_2
#if !defined(CONFIG_PWM1_2_IO)
#error "CONFIG_PWM1_2_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM1_2		10
#endif
#if CONFIG_PWM1_3
#if CONFIG_PWM1_3_IO
#error "PWM1_3 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_3_IO undefined or set to 0"
#endif
#define PWM1_3		11
#endif
#if CONFIG_PWM1_4
#if CONFIG_PWM1_4_IO
#error "PWM1_4 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_4_IO undefined or set to 0"
#endif
#define PWM1_4		12
#endif
#if CONFIG_PWM1_5
#if CONFIG_PWM1_5_IO
#error "PWM1_5 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_5_IO undefined or set to 0"
#endif
#define PWM1_5		13
#endif
#if CONFIG_PWM1_6
#if CONFIG_PWM1_6_IO
#error "PWM1_6 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_6_IO undefined or set to 0"
#endif
#define PWM1_6		14
#endif
#if CONFIG_PWM1_7
#if CONFIG_PWM1_7_IO
#error "PWM1_7 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM1_7_IO undefined or set to 0"
#endif
#define PWM1_7		15
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
#define PWM2_0		16
#endif
#if CONFIG_PWM2_1
#if !defined(CONFIG_PWM2_1_IO)
#error "CONFIG_PWM2_1_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM2_1		17
#endif
#if CONFIG_PWM2_2
#if !defined(CONFIG_PWM2_2_IO)
#error "CONFIG_PWM2_2_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM2_2		18
#endif
#if CONFIG_PWM2_3
#if CONFIG_PWM2_3_IO
#error "PWM2_3 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM2_3_IO undefined or set to 0"
#endif
#define PWM2_3		19
#endif
#if CONFIG_PWM2_4
#if CONFIG_PWM2_4_IO
#error "PWM2_4 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM2_4_IO undefined or set to 0"
#endif
#define PWM2_4		20
#endif
#if CONFIG_PWM2_5
#if CONFIG_PWM2_5_IO
#error "PWM2_5 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM2_5_IO undefined or set to 0"
#endif
#define PWM2_5		21
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
#define PWM3_0		24
#endif
#if CONFIG_PWM3_1
#if !defined(CONFIG_PWM3_1_IO)
#error "CONFIG_PWM3_1_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM3_1		25
#endif
#if CONFIG_PWM3_2
#if !defined(CONFIG_PWM3_2_IO)
#error "CONFIG_PWM3_2_IO should define the GPIO pin used as the PWM output"
#endif
#define PWM3_2		26
#endif
#if CONFIG_PWM3_3
#if CONFIG_PWM3_3_IO
#error "PWM3_3 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM3_3_IO undefined or set to 0"
#endif
#define PWM3_3		27
#endif
#if CONFIG_PWM3_4
#if CONFIG_PWM3_4_IO
#error "PWM3_4 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM3_4_IO undefined or set to 0"
#endif
#define PWM3_4		28
#endif
#if CONFIG_PWM3_5
#if CONFIG_PWM3_5_IO
#error "PWM3_5 GPIO pin is fixed and cannot be changed. Leave CONFIG_PWM3_5_IO undefined or set to 0"
#endif
#define PWM3_5		29
#endif
#endif

/// @brief: Returns the module number of pwm channel.
#define PWM_GET_MODULE(pwm)		(pwm / 8)
/// @brief: Returns the channel number from the module (0-7).
#define PWM_GET_CHANNEL(pwm)	(pwm % 8)


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
#if CONFIG_TARGET_LPC1549
typedef volatile uint8_t uv_pwm_channel_t;
#elif CONFIG_TARGET_LPC1785
typedef volatile uint32_t * uv_pwm_channel_t;
#endif


#define PWM_MAX_VALUE		1000U

/// @brief: Can be used to adjust the PWM duty cycle regardless of the MAX value
#define DUTY_CYCLE(x_float)	(uint16_t)((float) PWM_MAX_VALUE * ((float) x_float))

#define DUTY_CYCLEPPT(ppt)	((uint32_t) PWM_MAX_VALUE * ppt / 1000)


/// @brief: Initializes the PWM modules
uv_errors_e _uv_pwm_init();

/// @brief: Sets the PWM channels output.
///
/// @param chn: The PWM channel to be set
uv_errors_e uv_pwm_set(uv_pwm_channel_t chn, uint16_t value);


/// @brief: Sets the PWM frequency. Note that all pwm channels from the same pwm module
/// share the same frequency. (PWM0_0, PWM0_1, PWM0_2, etc)
void uv_pwm_set_freq(uv_pwm_channel_t chn, uint32_t value);

/// @brief: Returns the current PWM value
uint16_t uv_pwm_get(uv_pwm_channel_t chn);


#endif

#endif /* UV_HAL_INC_UV_PWM_H_ */
