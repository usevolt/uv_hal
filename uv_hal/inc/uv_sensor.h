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


#if CONFIG_ADC0 || CONFIG_ADC1

#define CONFIG_SENSOR		1

/// @file: Module for input sensor data.

enum {
	SENSOR_STATE_OK,
	SENSOR_STATE_WARNING,
	SENSOR_STATE_FAULT
};
typedef uint8_t uv_sensor_state_e;

#define UV_SENSOR_VALS_COUNT	5

/// @file: Implements a module for reading analog sensor data
typedef struct {
	uv_moving_aver_st avg;
	int16_t value;
	int16_t val_buffer[UV_SENSOR_VALS_COUNT];
	uv_ring_buffer_st vals;
	uv_sensor_state_e state;
	struct {
		uv_hysteresis_st hyst;
		union {
			uint32_t emcy;
			uint32_t enabled;
		};
	} warning;
	struct {
		uv_hysteresis_st hyst_high;
		uv_hysteresis_st hyst_low;
		union {
			uint32_t emcy;
			uint32_t enabled;
		};
	} fault;
	uv_adc_channels_e adc;
	int16_t (*get_data)(uv_adc_channels_e chn);
} uv_sensor_st;



/// @brief: Initializes the sensor module
void uv_sensor_init(uv_sensor_st *this, uv_adc_channels_e adc_chn, uint16_t avg_count,
		int16_t (*get_data)(uv_adc_channels_e chn));



/// @brief: Sets the fault limits
///
/// @param min_value: The value which is used as a minimum fault value
/// @param max_value: The value which is used as a maximum fault value
/// @param hysteresis: Hysteresis around the trigger values
void uv_sensor_set_fault(uv_sensor_st *this, int16_t min_value, int16_t max_value,
		int16_t hysteresis, uint32_t emcy_fault);

/// @brief: Sets the warning limit
///
/// @param trigger_value: The value which is used as a trigger value
/// @param hysteresis: Hysteresis around the trigger value
/// @param invert: true if warning is generated when value goes below **trigger_value**,
/// false if warning should be generated when value goes over **trigger_value**
void uv_sensor_set_warning(uv_sensor_st *this, int16_t trigger_value, int16_t hysteresis,
		bool invert, uint32_t emcy_warning);


/// @brief: Step function
void uv_sensor_step(uv_sensor_st *this, uint16_t step_ms);

/// @brief: Returns the current value
static inline int16_t uv_sensor_get_value(const uv_sensor_st *this) {
	return this->value;
}

/// @brief: Returns the current state
static inline uv_sensor_state_e uv_sensor_get_state(const uv_sensor_st *this) {
	return this->state;
}

#endif

#endif /* SENSOR_H_ */
