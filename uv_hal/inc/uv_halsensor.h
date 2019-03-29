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


#ifndef HAL_UV_HAL_INC_UV_HALSENSOR_H_
#define HAL_UV_HAL_INC_UV_HALSENSOR_H_

#include "uv_utilities.h"
#include "uv_canopen.h"
#include "uv_adc.h"
#include "uv_filters.h"
#include <uv_hal_config.h>

/// @file: Module for reading HAL sensors with ADC channel


#define HALSENSOR_CONFIG_MIN_SUBINDEX				1
#define HALSENSOR_CONFIG_MAX_SUBINDEX				2
#define HALSENSOR_CONFIG_MIDDLE_SUBINDEX			3
#define HALSENSOR_CONFIG_MIDDLE_TOLERANCE_SUBINDEX	4
#define HALSENSOR_CONFIG_SUBINDEX_COUNT				5

#if CONFIG_HALSENSOR


#define HALSENSOR_AVG_COUNT		1

#if !defined(CONFIG_HALSENSOR_MIN_DEF)
#define CONFIG_HALSENSOR_MIN_DEF				(ADC_MAX_VALUE / 2)
#endif
#if !defined(CONFIG_HALSENSOR_MAX_DEF)
#define CONFIG_HALSENSOR_MAX_DEF				(ADC_MAX_VALUE / 2)
#endif
#if !defined(CONFIG_HALSENSOR_MIDDLE_DEF)
#define CONFIG_HALSENSOR_MIDDLE_DEF				(ADC_MAX_VALUE / 2)
#endif
#if !defined(CONFIG_HALSENSOR_MIDDLE_TOLERANCE_DEF)
#define CONFIG_HALSENSOR_MIDDLE_TOLERANCE_DEF	(ADC_MAX_VALUE / 40)
#endif


/// @brief: HAL sensor states
enum {
	// sensor is operational
	HALSENSOR_STATE_ON = 0,
	// sensor is in a fault state
	HALSENSOR_STATE_FAULT,
	// sensor is in calibration
	HALSENSOR_STATE_CALIBRATION,
	HALSENSOR_STATE_COUNT
};
typedef uint8_t halsensor_state_e;



typedef struct {
	uint16_t min;
	uint16_t max;
	uint16_t middle;
	// tolerance around the middle value
	uint16_t middle_tolerance;
} uv_halsensor_config_st;

/// @brief: resets the configuration structure to default values
void uv_halsensor_config_reset(uv_halsensor_config_st *this);


#define HALSENSOR_OUTPUT_MAX	INT8_MAX
#define HALSENSOR_OUTPUT_MIN	(INT8_MIN + 1)

/// @brief: The main hal sensor module
typedef struct {
	// adc channel for reading the sensor value
	uv_adc_channels_e adc_chn;
	// moving average filter for the input adc
	uv_moving_aver_st moving_aver;

	halsensor_state_e state;

	// output value, between range INTX_MIN + 1 ... INTX_MAX, where X is 8, 16 or 32.
	// if fault is detected, output is driven to 0.
	int8_t output8;
	int16_t output16;
	int32_t output32;

	// output raw value in millivolts
	uint16_t out_mv;

	// emcy which will be sent if the sensor goes into fault mode
	uint32_t fault_emcy;

	// pointer to configuration structure which should be stored in non-volatile memory
	uv_halsensor_config_st *config;

} uv_halsensor_st;


/// @brief: Initializes the module
void uv_halsensor_init(uv_halsensor_st *this, uv_halsensor_config_st *config,
		uv_adc_channels_e adc_chn, uint32_t fault_emcy);


/// @brief: Step function should be called every step cycle
///
/// @return: output value. Can be also fetched with *uv_halsensor_get_output32*
int32_t uv_halsensor_step(uv_halsensor_st *this, uint16_t step_ms);


/// @brief: Puts the hal sensor into calibration state or out from it
void uv_halsensor_set_calbration(uv_halsensor_st *this, bool value);


static inline halsensor_state_e uv_halsensor_get_state(uv_halsensor_st *this) {
	return this->state;
}

/// @brief: Returns the output from the HAL sensor module in 8-bit integer
static inline int8_t uv_halsensor_get_output8(uv_halsensor_st *this) {
	return this->output8;
}

/// @brief: Returns the output from the HAL sensor module in 16-bit integer
static inline int16_t uv_halsensor_get_output16(uv_halsensor_st *this) {
	return this->output16;
}

/// @brief: Returns the output from the HAL sensor module in 32-bit integer
static inline int32_t uv_halsensor_get_output32(uv_halsensor_st *this) {
	return this->output32;
}

/// @brief: Returns the output absolute voltage in millivolts
static inline uint16_t uv_halsensor_get_out_mv(uv_halsensor_st *this) {
	return this->out_mv;
}



#endif

#endif /* HAL_UV_HAL_INC_UV_HALSENSOR_H_ */
