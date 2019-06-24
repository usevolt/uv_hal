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


#include "uv_sensor.h"

#if CONFIG_SENSOR


void uv_sensor_init(uv_sensor_st *this, uv_adc_channels_e adc_chn, uint16_t avg_count,
		int16_t (*get_data)(uv_adc_channels_e chn)) {
	this->value = 0;
	this->adc = adc_chn;
	uv_moving_aver_init(&this->avg, avg_count);
	this->fault.emcy = 0;
	this->warning.emcy = 0;
	this->get_data = get_data;
	this->state = SENSOR_STATE_OK;
}


void uv_sensor_set_warning(uv_sensor_st *this, int16_t trigger_value, int16_t hysteresis,
		bool invert, uint32_t emcy_warning) {
	uv_hysteresis_init(&this->warning.hyst, trigger_value, hysteresis, invert);
	this->warning.emcy = emcy_warning;
}


void uv_sensor_set_fault(uv_sensor_st *this, int16_t min_value, int16_t max_value,
		int16_t hysteresis, uint32_t emcy_fault) {
	uv_hysteresis_init(&this->fault.hyst_low, min_value, hysteresis, true);
	uv_hysteresis_init(&this->fault.hyst_high, max_value, hysteresis, false);
	this->fault.emcy = emcy_fault;
}



void uv_sensor_step(uv_sensor_st *this, uint16_t step_ms) {
	this->value = uv_moving_aver_step(&this->avg, this->get_data(this->adc));

	if (uv_moving_aver_is_full(&this->avg)) {
		if (this->warning.enabled &&
				((this->state == SENSOR_STATE_OK) ||
						(this->state == SENSOR_STATE_WARNING))) {
			if (uv_hysteresis_step(&this->warning.hyst, this->value)) {
				if (this->state != SENSOR_STATE_WARNING) {
					uv_canopen_emcy_send(CANOPEN_EMCY_DEVICE_SPECIFIC, this->warning.emcy);
				}
				this->state = SENSOR_STATE_WARNING;
			}
			else {
				this->state = SENSOR_STATE_OK;
			}
		}

		if (this->fault.enabled) {
			if (uv_hysteresis_step(&this->fault.hyst_high, this->value) ||
					uv_hysteresis_step(&this->fault.hyst_low, this->value)) {
				if (this->state != SENSOR_STATE_FAULT) {
					uv_canopen_emcy_send(CANOPEN_EMCY_DEVICE_SPECIFIC, this->fault.emcy);
				}
				this->state = SENSOR_STATE_FAULT;
			}
			else {
				this->state = uv_hysteresis_get_output(&this->warning.hyst) ?
						SENSOR_STATE_WARNING : SENSOR_STATE_OK;

			}
		}
	}
}


#endif
