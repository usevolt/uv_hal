/*
 * uw_timer.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef UW_TIMER_H_
#define UW_TIMER_H_


#include "uw_hal_config.h"

#include "stdbool.h"
#include "stdint.h"
#include "uw_errors.h"
#include "uw_gpio.h"
#include <stdio.h>



/// @brief: Enum describing all different interrupt sources which the timers can trigger
typedef enum {
	INT_SRC_OVERFLOW,
	INT_SRC_CAPTURE0,
#if CONFIG_TARGET_LPC178X
	INT_SRC_CAPTURE1,
#endif
	INT_SRC_COUNTER,
	INT_SRC_COUNT
} uw_timer_int_sources_e;


/// @brief: Defines match outputs for pwm signals
/// @note: MATCH 3 is used as an overflow detection on all timers
typedef enum {
#if CONFIG_TARGET_LPC11CXX
	MATCH_0 = 0,
	MATCH_1,
	MATCH_2,
	MATCH_COUNT
#elif CONFIG_TARGET_LPC178X
	MATCH_0 = 0,
	MATCH_1,
	MATCH_2,
	MATCH_3,
	MATCH_COUNT
#endif
} uw_timer_matches_e;


/// @brief: Defines for different pwm channels
typedef enum {
	PWM_CHANNEL_0 = 0,
	PWM_CHANNEL_1,
	PWM_CHANNEL_2,
#if CONFIG_TARGET_LPC11CXX
	PWM_CHANNEL_COUNT = 3
#elif CONFIG_TARGET_LPC178X
	PWM_CHANNEL_3,
	PWM_CHANNEL_4,
	PWM_CHANNEL_5,
	PWM_CHANNEL_COUNT = 6
#endif
} uw_pwm_channels_e;


/// @brief: Defines the capture inputs on this hardware
typedef enum {
	CAPTURE0,
#if CONFIG_TARGET_LPC178X
	CAPTURE1,
#endif
	CAPTURE_COUNT
} uw_timer_captures_e;


typedef enum {
#if CONFIG_TARGET_LPC11CXX
	CAPTURE_RISING_EDGE = 0,
	CAPTURE_FALLING_EDGE,
	CAPTURE_BOTH_EDGES,
	CAPTURE_MODE_COUNT
#elif CONFIG_TARGET_LPC178X
	CAPTURE_RISING_EDGE = 0,
	CAPTURE_FALLING_EDGE,
	CAPTURE_BOTH_EDGES,
	CAPTURE_MODE_COUNT
#endif
} uw_timer_capture_modes_e;


/// @brief: Defines timers available in this hardware
typedef enum {
#if CONFIG_TIMER0
	TIMER0 = 0,
#endif
#if CONFIG_TIMER1
	TIMER1 = 1,
#endif
#if CONFIG_TIMER2
	TIMER2 = 2,
#endif
#if CONFIG_TIMER3
	TIMER3 = 3,
#endif
	TIMER_COUNT = 4
} uw_timers_e;

/// @brief: Defines for counters
enum {
#if CONFIG_COUNTER0
	COUNTER0 = 0,
#endif
#if CONFIG_COUNTER1
	COUNTER1 = 1,
#endif
#if CONFIG_COUNTER2
	COUNTER2 = 2,
#endif
#if CONFIG_COUNTER3
	COUNTER3 = 3,
#endif
	COUNTER_COUNT = 4
};

/// @brief: Defines PWM modules available on this hardware
typedef enum {
#if CONFIG_PWM0
	PWM0 = 0,
#endif
#if CONFIG_PWM1
	PWM1 = 1,
#endif
#if (CONFIG_TARGET_LPC11CXX && CONFIG_PWM2)
	PWM2 = 2,
#endif
#if (CONFIG_TARGET_LPC11CXX && CONFIG_PWM3)
	PWM3 = 3,
#endif
#if CONFIG_TARGET_LPC178X
	PWM_COUNT = 2
#elif CONFIG_TARGET_LPC11CXX
	PWM_COUNT = 4
#endif
} uw_pwms_e;


/// @brief: Enum for all different capture logical edges possible for timers
typedef enum {
	TIMER_CAPTURE_RISING_EDGE = 0,
	TIMER_CAPTURE_FALLING_EDGE,
	TIMER_CAPTURE_BOTH_EDGES
} uw_timer_capture_edges_e;





/// @brief: initializes a timer/counter module as a timer but doesn't start it.
/// To use the timer/counter module as a module, use uw_counter_init instead.
///
/// @note: This function should be called only once per application. Reinitializing
/// the timer may cause a hard fault interrupt!
///
/// @return: error enum defined in uw_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
/// @param cycle_freq: timer overflow loop cycle frequency in Hz
uw_errors_e uw_timer_init(uw_timers_e timer, float cycle_freq);






/// @brief: initializes a timer/counter module as a counter.
/// To use the timer/counter module as a module, use uw_counter_init instead.
///
/// @note: This function should be called only once per application. Reinitializing
/// the timer may cause a hard fault interrupt!
///
/// @return: error enum defined in uw_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
uw_errors_e uw_counter_init(uw_timers_e timer);






/// @brief: Initializes a timer/counter module as a pwm output and starts it.
/// To use the module as a generic timer or counter, use uw_timer_init or
/// uw_counter_init functions instead.
///
/// @note: This function should be called only once per application. Reinitializing
/// the timer may cause a hard fault interrupt!
///
/// @return: error enum defined in uw_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param pwm: Selects the timer/pwm module which will be initialized. Available values depend
/// on hardware used.
/// @param: pwm_channels: OR'red PWM channels to be initialized.
/// The PWM channels are initialized with a 0 % duty cycle. To change the channel
/// configuration, call uw_pwm_conf_channel function.
/// @param freq: The frequency of the PWM channels in Hz. All channels use the same frequency.
static inline uw_errors_e uw_pwm_init(uw_pwms_e pwm, uw_pwm_channels_e channels,
		unsigned int freq) {
				return uw_timer_init(pwm, freq);
};






/// @brief: Starts the timer from the timer current value
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
void uw_timer_start(uw_timers_e timer);




/// @brief: Stops the timer without clearing the timer register
///
/// @return: error enum defined in uw_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
void uw_timer_stop(uw_timers_e timer);





/// @brief: Resets the timer to zero
///
/// @return: error enum defined in uw_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
void uw_timer_clear(uw_timers_e timer);
static inline void uw_counter_clear(uw_timers_e timer) { uw_timer_clear(timer); }





/// @brief: Set's the timer's overflow frequency. Can be used to change a once initialized
/// timer's frequency.
///
/// @note: Doesn't stop or clear the timer. If the timer is running when calling this function,
/// the timer increment value is updated on-the-go.
///
/// @return: error enum defined in uw_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
/// @param cycle_freq: timer overflow loop cycle frequency in Hz
uw_errors_e uw_timer_set_freq(uw_timers_e timer, float freq);






/// @brief: get the timer's current value
///
/// @return: error enum defined in uw_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
int uw_timer_get_value(uw_timers_e timer);
static inline int uw_counter_get_value(uw_timers_e timer) { return uw_timer_get_value(timer); }






/// @brief: Sets the pwm channel's duty cycle
///
/// @pre: The timer is initialized with uw_pwm_init function
///
/// @return: error enum defined in uw_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param pwm: Selects the timer/pwm module which will be initialized. Available values depend
/// on hardware used.
/// @param channel: the used timer's PWM channel to be configured. Available values are
/// hardware dependant
/// @param duty_cycle: The PWM output's duty cycle. Value should be between 0 and 1000.
/// Other values are clamped to valid range and a warning message is logged to stdout.
uw_errors_e uw_pwm_set(uw_pwms_e pwm, uw_pwm_channels_e channel, unsigned int duty_cycle);








/// @brief: register callback function to be executed when delay_ms mseconds have passed
///
/// @return: error enum defined in uw_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
uw_errors_e uw_timer_add_callback(uw_timers_e timer, void (*callback_function)
		(void* user_ptr, uw_timers_e timer, uw_timer_int_sources_e source, unsigned int value));
static inline uw_errors_e uw_counter_add_callback(uw_timers_e timer, void (*callback_function)
		(void* user_ptr, uw_timers_e counter, uw_timer_int_sources_e source, unsigned int value)) {
	return uw_timer_add_callback(timer, callback_function);
}




#if !CONFIG_RTOS
/// @brief: Initializes the Cortex tick timer
///
/// @return: error enum defined in uw_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param freq: The frequency in which the tick timer will cause interrupts inHZ
uw_errors_e uw_tick_timer_init(uint32_t freq);

/// @brief: Adds a callback to tick timer interrupt
void uw_tick_timer_add_callback(void (*callback_function_ptr)(void* user_ptr, uint32_t));
#endif




/**** ERRORS OF HARDWARE COMPATIBILITY ******/

//#if (CONFIG_TIMER0 + CONFIG_COUNTER0 > 1)
//#error "Capture0, timer0 and counter0 cannot be enabled at the same time."
//#endif
//#if (CONFIG_TIMER1 + CONFIG_COUNTER1 > 1)
//#error "Capture1, timer1 and counter1 cannot be enabled at the same time."
//#endif
//#if (CONFIG_TIMER2 + CONFIG_COUNTER2 > 1)
//#error "Capture2, timer2 and counter2 cannot be enabled at the same time."
//#endif
//#if (CONFIG_TIMER3 + CONFIG_COUNTER3 > 1)
//#error "Capture3, timer3 and counter3 cannot be enabled at the same time."
//#endif
//#if CONFIG_TARGET_LPC11CXX
//#if (CONFIG_PWM0 + CONFIG_TIMER0 + CONFIG_COUNTER0 > 1)
//#error "Capture0, counter0, timer0 and pwm0 cannot be enabled at the same time."
//#endif
//#if (CONFIG_PWM1 + CONFIG_TIMER1 + CONFIG_COUNTER1 > 1)
//#error "Capture1, counter1, timer1 and pwm1 cannot be enabled at the same time."
//#endif
//#if (CONFIG_PWM2 + CONFIG_TIMER2 + CONFIG_COUNTER2 > 1)
//#error "Capture2, counter2, timer2 and pwm2 cannot be enabled at the same time."
//#endif
//#if (CONFIG_PWM3 + CONFIG_TIMER3 + CONFIG_COUNTER3 > 1)
//#error "Capture3, counter3, timer3 and pwm3 cannot be enabled at the same time."
//#if (CONFIG_PWM0_CHANNEL4 || CONFIG_PWM0_CHANNEL5 || CONFIG_PWM0_CHANNEL6)
//#error "PWM0 has only 3 channels."
//#endif
//#if (CONFIG_PWM1_CHANNEL4 || CONFIG_PWM1_CHANNEL5 || CONFIG_PWM1_CHANNEL6)
//#error "PWM1 has only 3 channels."
//#endif
//#if (CONFIG_PWM2_CHANNEL4 || CONFIG_PWM2_CHANNEL5 || CONFIG_PWM2_CHANNEL6)
//#error "PWM2 has only 3 channels."
//#endif
//#if (CONFIG_PWM3_CHANNEL4 || CONFIG_PWM3_CHANNEL5 || CONFIG_PWM3_CHANNEL6)
//#error "PWM3 has only 3 channels."
//#endif
//#endif
//#elif CONFIG_TARGET_LPC178X
//#if (CONFIG_PWM2 || CONFIG_PWM3)
//#error "PWM modules 2 and 3 not available on LPC178x."
//#endif
//#if (CONFIG_PWM2 || CONFIG_PWM3)
//#error "LPC178x has only 2 PWM modules."
//#endif
//#endif


#endif /* UW_TIMER_H_ */
