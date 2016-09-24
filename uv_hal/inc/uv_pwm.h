/*
 * uv_pwm.h
 *
 *  Created on: Sep 19, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UV_PWM_H_
#define UV_HAL_INC_UV_PWM_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif


#if CONFIG_PWM


#if !defined(CONFIG_PWM_FREQ)
#error "CONFIG_PWM_FREQ should define the PWM frequency in Hz"
#endif

#if CONFIG_TARGET_LPC1785
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
#endif
#if CONFIG_PWM0_2
#define PWM0_2				&LPC_PWM0->MR2
#endif
#if CONFIG_PWM0_3
#define PWM0_3				&LPC_PWM0->MR3
#endif
#if CONFIG_PWM0_4
#define PWM0_4				&LPC_PWM0->MR4
#endif
#if CONFIG_PWM0_5
#define PWM0_5				&LPC_PWM0->MR5
#endif
#if CONFIG_PWM0_6
#define PWM0_6				&LPC_PWM0->MR6
#endif
#if CONFIG_PWM1_1
#define PWM1_1				&LPC_PWM1->MR1
#endif
#if CONFIG_PWM1_2
#define PWM1_2				&LPC_PWM1->MR2
#endif
#if CONFIG_PWM1_3
#define PWM1_3				&LPC_PWM1->MR3
#endif
#if CONFIG_PWM1_4
#define PWM1_4				&LPC_PWM1->MR4
#endif
#if CONFIG_PWM1_5
#define PWM1_5				&LPC_PWM1->MR5
#endif
#if CONFIG_PWM1_6
#define PWM1_6				&LPC_PWM1->MR6
#endif
/// @brief: Variable to separate different PWM channels from each other
/// Possible values are PWM channel macros defined upper.
typedef volatile uint32_t* uv_pwm_channel_t;



#define PWM_MAX_VALUE		1000.0f

/// @brief: Can be used to adjust the PWM duty cycle regardless of the MAX value
#define DUTY_CYCLE(x_float)	(uint16_t)(PWM_MAX_VALUE * (float) x_float)


/// @brief: Initializes the PWM modules
uv_errors_e uv_pwm_init();

/// @brief: Sets the PWM channels output.
///
/// @param chn: The PWM channel to be set
/// @param value: The PWM value is 16-bit unsigned value between 0 ... 65536
static inline uv_errors_e uv_pwm_set(uv_pwm_channel_t chn, uint16_t value) {
	if (value >= PWM_MAX_VALUE) {
		value = PWM_MAX_VALUE;
	}
	*chn = PWM_MAX_VALUE - value;
#if CONFIG_PWM0
	LPC_PWM0->LER = 0x7F;
#endif
#if CONFIG_PWM1
	LPC_PWM1->LER = 0x7F;
#endif

	return uv_err(ERR_NONE);
}


#else
#error "This target doesn't have a PWM peripheral"
#endif

#endif

#endif /* UV_HAL_INC_UV_PWM_H_ */
