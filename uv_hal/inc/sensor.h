/*
 * sensor.h
 *
 *  Created on: Sep 7, 2017
 *      Author: usevolt
 */

#ifndef SENSOR_H_
#define SENSOR_H_

#include "uv_utilities.h"
#include "uv_filters.h"
#include "uv_adc.h"
#include "uv_canopen.h"


/// @file: Module for input sensor data.

enum {
	SENSOR_STATE_OK,
	SENSOR_STATE_WARNING,
	SENSOR_STATE_ERROR,
	SENSOR_STATE_FAULT
};
typedef uint8_t sensor_state_e;


/// @file: Implements a module for reading analog sensor data
typedef struct {
	uv_moving_aver_st avg;
	int8_t value;
	bool hyst_warning_enabled;
	bool hyst_error_enabled;
	sensor_state_e state;
	uv_hysteresis_st hyst_warning;
	esb_emcy_e emcy_warning;
	uv_hysteresis_st hyst_error;
	esb_emcy_e emcy_error;
	esb_emcy_e emcy_fault;
	uv_adc_channels_e adc;
	bool (*get_data)(uv_adc_channels_e chn, int8_t *dest);
} sensor_st;



/// @brief: Initializes the sensor module
void sensor_init(sensor_st *this, uv_adc_channels_e adc_chn, uint16_t avg_count,
		esb_emcy_e emcy_fault, bool (*get_data_ptr)(uv_adc_channels_e chn, int8_t *dest));

/// @brief: Sets the warning limit
///
/// @param trigger_value: The value which is used as a trigger value
/// @param hysteresis: Hysteresis around the trigger value
/// @param invert: true if warning is generated when value goes below **trigger_value**,
/// false if warning should be generated when value goes over **trigger_value**
void sensor_set_warning(sensor_st *this, int8_t trigger_value, int8_t hysteresis,
		bool invert, esb_emcy_e emcy_warning);


/// @brief: Sets the error limit
///
/// @param trigger_value: The value which is used as a trigger value
/// @param hysteresis: Hysteresis around the trigger value
/// @param invert: true if error is generated when value goes below **trigger_value**,
/// false if error should be generated when value goes over **trigger_value**
void sensor_set_error(sensor_st *this, int8_t trigger_value, int8_t hysteresis,
		bool invert, esb_emcy_e emcy_error);

/// @brief: Step function
void sensor_step(sensor_st *this, uint16_t step_ms);

/// @brief: Returns the current value
static inline int8_t sensor_get_value(const sensor_st *this) {
	return this->value;
}

/// @brief: Returns the current state
static inline sensor_state_e sensor_get_state(const sensor_st *this) {
	return this->state;
}



#endif /* SENSOR_H_ */
