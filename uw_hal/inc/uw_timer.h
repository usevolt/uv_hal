/*
 * uw_timer.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef UW_TIMER_H_
#define UW_TIMER_H_


#include "stdbool.h"
#include "stdint.h"
#include <stdio.h>


/// @brief: Enum describing all different interrupt sources which the timers can trigger
typedef enum {
	INT_SRC_OVERFLOW,
	INT_SRC_CAPTURE,
	INT_SRC_COUNTER,
	INT_SRC_COUNT
} uw_timer_int_sources_e;


/// @brief: Defines match outputs for pwm signals
/// @note: MATCH 3 is used as an overflow detection on all timers
typedef enum {
#ifdef LPC11C14
	MATCH_0 = 0,
	MATCH_1,
	MATCH_2,
	MATCH_COUNT
#elif defined(LPC1785)
#warning "Controller not yet defined!"
#endif
} uw_timer_matches_e;


/// @brief: Defines for different pwm channels
typedef enum {
#ifdef LPC11C14
	PWM_CHANNEL_0 = (1 << 0),
	PWM_CHANNEL_1 = (1 << 1),
	PWM_CHANNEL_2 = (1 << 2)
#elif defined(LPC1785)
#warning "Controller not yet defined!"
#endif
} uw_timer_pwms_e;

typedef enum {
#ifdef LPC11C14
	CAPTURE_0_RISING_EDGE = 0,
	CAPTURE_0_FALLING_EDGE,
	CAPTURE_0_BOTH_EDGES,
	CAPTURE_COUNT
#elif defined(LPC1785)
#warning "Controller not yet defined!"
#endif
} uw_timer_captures_e;

/// @brief: Defines timers available in this hardware
typedef enum {
#ifdef LPC11C14
	CT16B0 = 0,
	CT16B1,
	CT32B0,
	CT32B1,
	TIMER_COUNT,
#elif defined(LPC1785)
#warning "Controller not yet defined!"
#endif
	// tick timer is not included in the timer count since it kind of lives it's own life
	TICK_TIMER
} uw_timers_e;



/// @brief: Describes the mode which can be used while initializing the timers
/// These values are only used while initializing the timers.
typedef enum {
	/// @brief: Value passed will be copied straight to the timer prescaler register
	TIMER_PRESCALER,
	/// @brief: Value passed describes the timer overflow frequency in Hz
	TIMER_OVERFLOW_FREQ_HZ,
	/// @brief: Value passed describes the timer step frequency in Hz
	TIMER_STEP_FREQ_HZ,
	/// @brief: Value passed describes the timer overflow duration in micro seconds
	TIMER_OVERFLOW_TIME_US,
	/// @brief: Value passed describes the timer step duration in micro seconds
	TIMER_STEP_TIME_US
} uw_timer_init_modes_e;


/// @brief: Enum for all different capture logical edges possible for timers
typedef enum {
	TIMER_CAPTURE_RISING_EDGE = 0,
	TIMER_CAPTURE_FALLING_EDGE,
	TIMER_CAPTURE_BOTH_EDGES
} uw_timer_capture_edges_e;



/// @brief: initializes a timer/counter module as a timer and starts it.
/// To use the timer/counter module as a module, use uw_counter_init instead.
///
/// @return: True if everything succeeded. False if error was detected. Info about the error
/// is logged into stdout.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
/// @param init_mode: Value passed determines what init_value is assumed to mean.
/// See uw_timer_init_modes_e for possible values
/// @param freq: timer overflow loop cycle frequency in Hz
/// @param fosc: system oscillator frequency in Hz
bool uw_timer_init(uw_timers_e timer, uw_timer_init_modes_e init_mode,
		unsigned int init_value, unsigned int fosc);


/// @brief: Initializes a timer/counter module as a pwm output and starts it.
/// To use the module as a generic timer or counter, use uw_timer_init or
/// uw_counter_init functions instead.
///
/// @return: True if everything succeeded. False if error was detected. Info about the error
/// is logged into stdout.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
/// @param: pwm_channels: OR'red PWM channels to be initialized.
/// The PWM channels are initialized with a 0 % duty cycle. To change the channel
/// configuration, call uw_pwm_conf_channel function.
/// @param freq: The frequency of the PWM channels in Hz. All channels use the same frequency.
bool uw_pwm_init(uw_timers_e timer, uw_timer_pwms_e channels, unsigned int freq, unsigned int fosc);



/// @brief: initializes a timer/counter module as a counter.
/// To use the timer/counter module as a module, use uw_counter_init instead.
///
/// @return: True if everything succeeded. False if error was detected. Info about the error
/// is logged into stdout.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
/// @param capture_input: Selects the input which is used as a coutner input. Available
/// values are hardware dependant.
bool uw_counter_init(uw_timers_e timer, uw_timer_captures_e capture_input,
		uw_timer_capture_edges_e capture_edge);



/// @brief: get the timer's current value
///
/// @return: The timer's value. If an invalid timer was given as a parameter, returns -1
/// and logs an error to stdout.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
int uw_timer_get_value(uw_timers_e timer);
static inline int uw_counter_get_value(uw_timers_e timer) { return uw_timer_get_value(timer); }


/// @brief: Resets the timer to zero
///
/// @return: True if everything succeeded. False if error was detected. Info about the error
/// is logged into stdout.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
bool uw_timer_clear(uw_timers_e timer);
static inline bool uw_counter_clear(uw_timers_e timer) { return uw_timer_clear(timer); }


/// @brief: Configures a capture input to cause an interrupt callback with the timer's
/// value. Configures the selected external capture pin to capture mode but
/// leaves all other gpio configuration values untouched.
///
/// @return: True if everything succeeded. False if error was detected. Info about the error
/// is logged into stdout.
///
/// @param timer:  Selects the timer which will be initialized. Available values depend
/// on hardware used.
/// @param input: The capture input used when capturing data
bool uw_timer_add_capture(uw_timers_e timer, uw_timer_captures_e input);


/// @brief: Sets the pwm channel's duty cycle
///
/// @pre: The timer is initialized with uw_pwm_init function
///
/// @return: True if pwm could be set. False if error was detected. Info about the error
/// is logged into stdout.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
/// @param channel: the used timer's PWM channel to be configured. Available values are
/// hardware dependant
/// @param duty_cycle: The PWM output's duty cycle. Value should be between 0 and 1000.
/// Other values are clamped to valid range and a warning message is logged to stdout.
bool uw_pwm_set(uw_timers_e timer, uw_timer_pwms_e channel, unsigned int duty_cycle);




/// @brief: register callback function to be executed when delay_ms mseconds have passed
///
/// @return: True if everything succeeded. False if error was detected. Info about the error
/// is logged into stdout.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
bool uw_timer_add_callback(uw_timers_e timer, void (*callback_function)
		(void* user_ptr, uw_timer_int_sources_e source, unsigned int value));
static inline bool uw_counter_add_callback(uw_timers_e timer, void (*callback_function)
		(void* user_ptr, uw_timer_int_sources_e source, unsigned int value)) {
	return uw_timer_add_callback(timer, callback_function);
}


/// @brief: Initializes the Cortex tick timer
/// @param freq: The frequency in which the tick timer will cause interrupts
/// @param fosc: The system clock frequency
bool uw_tick_timer_init(uint32_t freq, uint32_t fosc);

/// @brief: Adds a callback to tick timer interrupt
void uw_tick_timer_add_callback(void (*callback_function_ptr)(void* user_ptr, uint32_t));


#endif /* UW_TIMER_H_ */
