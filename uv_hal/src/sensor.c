/*
 * sensor.c
 *
 *  Created on: Sep 7, 2017
 *      Author: usevolt
 */


#include "sensor.h"


void sensor_init(sensor_st *this, uv_adc_channels_e adc_chn, uint16_t avg_count,
		esb_emcy_e emcy_fault, bool (*get_data_ptr)(uv_adc_channels_e chn, int8_t *dest)) {
	this->value = 0;
	this->adc = adc_chn;
	uv_moving_aver_init(&this->avg, avg_count);
	this->emcy_fault = emcy_fault;
	this->emcy_error = 0;
	this->emcy_warning = 0;
	this->get_data = get_data_ptr;
	this->hyst_warning_enabled = false;
	this->hyst_error_enabled = false;
	this->state = SENSOR_STATE_OK;
}


void sensor_set_warning(sensor_st *this, int8_t trigger_value, int8_t hysteresis,
		bool invert, esb_emcy_e emcy_warning) {
	uv_hysteresis_init(&this->hyst_warning, trigger_value, hysteresis, invert);
	this->emcy_warning = emcy_warning;
	this->hyst_warning_enabled = true;
}


void sensor_set_error(sensor_st *this, int8_t trigger_value, int8_t hysteresis,
		bool invert, esb_emcy_e emcy_error) {
	uv_hysteresis_init(&this->hyst_error, trigger_value, hysteresis, invert);
	this->emcy_error = emcy_error;
	this->hyst_error_enabled = true;
}


void sensor_step(sensor_st *this, uint16_t step_ms) {
	if (this->get_data(this->adc, &this->value)) {
		this->value = uv_moving_aver_step(&this->avg, this->value);
		sensor_state_e state = SENSOR_STATE_OK;
		if (this->hyst_warning_enabled) {
			if (uv_hysteresis_step(&this->hyst_warning, this->value)) {
				if ((this->state != SENSOR_STATE_WARNING) &&
						(this->state != SENSOR_STATE_ERROR)) {
					uv_canopen_emcy_send(CANOPEN_EMCY_DEVICE_SPECIFIC, this->emcy_warning);
				}
				state = SENSOR_STATE_WARNING;
			}
		}
		if (this->hyst_error_enabled) {
			if (uv_hysteresis_step(&this->hyst_error, this->value)) {
				if (this->state != SENSOR_STATE_ERROR) {
					uv_canopen_emcy_send(CANOPEN_EMCY_DEVICE_SPECIFIC, this->emcy_error);
				}
				state = SENSOR_STATE_ERROR;
			}
		}
		this->state = state;
	}
	else {
		// sensor fault error
		if (this->state != SENSOR_STATE_FAULT) {
			uv_canopen_emcy_send(CANOPEN_EMCY_DEVICE_SPECIFIC, this->emcy_fault);
		}
		this->state = SENSOR_STATE_FAULT;
		this->value = 0;
		uv_moving_aver_reset(&this->avg);
	}
}

