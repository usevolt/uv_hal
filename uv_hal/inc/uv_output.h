/*
 * uv_output.h
 *
 *  Created on: Oct 10, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UV_OUTPUT_H_
#define UV_HAL_INC_UV_OUTPUT_H_



// @file: Power output module

#include <uv_hal_config.h>
#include <uv_utilities.h>
#include <uv_filters.h>
#include <uv_adc.h>
#include <uv_gpio.h>
#if CONFIG_CANOPEN
#include <uv_canopen.h>
#endif


#if !defined(CONFIG_OUTPUT)
#error "CONFIG_OUTPUT should be defined as 1 or 0, depending if uv_output module is used"
#endif


/// @brief: Defines the state of a single thruster power supply
enum {
	OUTPUT_STATE_OFF = 0,
	OUTPUT_STATE_ON,
	OUTPUT_STATE_OVERLOAD,
	OUTPUT_STATE_FAULT
};
typedef uint8_t uv_output_state_e;





typedef struct {
	/// @brief: ADC channel of the sense resistor
	uv_adc_channels_e adc_chn;
	/// @brief: Current sense resistor value in milliohms
	uint16_t sense_mohm;
	/// @brief: Current sense resistor amplification (defaults to 50)
	uint16_t sense_ampl;
	/// @brief: Current min limit in milliamps
	uint16_t limit_min;
	/// @brief: Current max limit in milliamps
	uint16_t limit_max;
	uv_moving_aver_st moving_avg;
	/// @brief: Holds the moving average output value
	uint16_t current;
	/// @brief: output module state
	uv_output_state_e state;
	/// @brief: gpio pin for the gate driving
	uv_gpios_e gate_io;
	/// @brief: EMCY messages to be triggered in overcurrent / undercurrent situations
	uint32_t emcy_overload;
	uint32_t emcy_fault;
} uv_output_st;



/// @brief: Initializes the output driver module
///
void uv_output_init(uv_output_st *this, const uv_adc_channels_e adc_chn, const uv_gpios_e gate_io,
		const uint16_t sense_mohm, const uint16_t min_val, const uint16_t max_val,
		const uint16_t moving_avg_count, uint32_t emcy_overload, uint32_t emcy_fault);


/// @brief: Sets the current sense amplification value. Defaults to 50
static inline void uv_output_set_ampl(uv_output_st *this, const uint16_t value) {
	this->sense_ampl = value;
}


static inline uv_output_state_e uv_output_get_state(const uv_output_st *this) {
	return this->state;
}

/// @brief: Sets the sense amplification. Defaults to 50.
static inline void uv_output_set_sense_ampl(uv_output_st *this, uint16_t sense_ampl) {
	this->sense_ampl = sense_ampl;
}


static inline uint16_t uv_output_get_current(uv_output_st *this) {
	return uv_moving_aver_get_val(&this->moving_avg);
}


/// @brief: Sets the output state
void uv_output_set_state(uv_output_st *this, const uv_output_state_e state);


/// @brief: Step function should be called every step cycle.
void uv_output_step(uv_output_st *this, uint16_t step_ms);







#endif /* UV_HAL_INC_UV_OUTPUT_H_ */
