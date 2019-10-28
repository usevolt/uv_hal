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

#include "uv_halsensor.h"


#if CONFIG_HALSENSOR

#define MAX_VAL_THRESHOLD	(ADC_MAX_VALUE / 20)

void uv_halsensor_config_reset(uv_halsensor_config_st *this) {
	this->max = CONFIG_HALSENSOR_MAX_DEF;
	this->min = CONFIG_HALSENSOR_MIN_DEF;
	this->middle = CONFIG_HALSENSOR_MIDDLE_DEF;
	this->middle_tolerance = CONFIG_HALSENSOR_MIDDLE_TOLERANCE_DEF;
	this->progression = HALSENSOR_PROG_2;
}


void uv_halsensor_init(uv_halsensor_st *this, uv_halsensor_config_st *config,
		uv_adc_channels_e adc_chn, uint32_t fault_emcy) {
	this->config = config;
	this->adc_chn = adc_chn;
	this->fault_emcy = fault_emcy;
	this->state = HALSENSOR_STATE_ON;
	uv_moving_aver_init(&this->moving_aver, HALSENSOR_AVG_COUNT);
}


static uint32_t get_progression_value(uint32_t input,
		uint32_t numberspace, halsensor_progression_e prog) {
	uint32_t ret = 0;

	switch (prog) {
	case HALSENSOR_PROG_0:
		ret = input;
		break;
	case HALSENSOR_PROG_1:
		ret = uv_isqrt(input * input * input) / uv_isqrt(numberspace);
		break;
	default:
		ret = input * input / numberspace;
		break;
	}
	LIMIT_MAX(ret, numberspace);

	return ret;
}


int32_t uv_halsensor_step(uv_halsensor_st *this, uint16_t step_ms) {
	uint16_t adc = uv_adc_read(this->adc_chn);
	adc = uv_moving_aver_step(&this->moving_aver, adc);
	this->out_adc = adc;

	// voltage output is always calculated
	this->out_mv = adc * 3300 / ADC_MAX_VALUE;

	if (this->state == HALSENSOR_STATE_ON) {
		// check if there's a fault in the input
		// fault limits are double of the config min & max values relative to ADC_MAX_VALUE
		// In case the max value is too close to ADC_MAX_VALUE, the upper limit is disabled
		int32_t max_val = (this->config->max > ADC_MAX_VALUE - MAX_VAL_THRESHOLD) ?
				ADC_MAX_VALUE + 1 : (this->config->max + (ADC_MAX_VALUE - this->config->max) / 2);
		if ((adc < this->config->min / 2) ||
				(adc > max_val)) {
			if (uv_moving_aver_is_full(&this->moving_aver)) {
				uv_canopen_emcy_send(CANOPEN_EMCY_DEVICE_SPECIFIC, this->fault_emcy);
			}
			this->state = HALSENSOR_STATE_FAULT;
			this->output16 = 0;
		}
		// calculate the output value
		else {
			// clamp adc to minimum and maximum
			if (adc < this->config->min) {
				adc = this->config->min;
			}
			else if (adc > this->config->max) {
				adc = this->config->max;
			}
			// positive side
			if (adc > (this->config->middle + this->config->middle_tolerance / 2)) {
				int32_t offset = this->config->middle + this->config->middle_tolerance / 2;
				int32_t scale = this->config->max - offset;
				if (scale != 0) {
					// scale adc from zero to config max
					adc -= offset;
					// apply the progression
					adc = get_progression_value(adc, scale, this->config->progression);
					// linearily interpolate output value
					int32_t rel = uv_reli(adc, 0, scale);
					int32_t result = uv_lerpi(rel, 0, INT16_MAX);
					if (result < 0) {
						result = 0;
					}
					else if (result > INT16_MAX) {
						result = INT16_MAX;
					}
					else {

					}
					this->output16 = result;
				}
				else {
					this->output16 = 0;
				}
			}
			// negative side
			else if (adc < (this->config->middle - this->config->middle_tolerance / 2)) {
				int32_t offset = this->config->middle - this->config->middle_tolerance / 2;
				int32_t scale = offset - this->config->min;
				if (scale != 0) {
					// scale adc from zero to positive upwards
					// this is a little more difficult than on positive side
					adc = abs(adc - offset);
					// apply second exponent
					adc = get_progression_value(adc, scale, this->config->progression);
					// linearily interpolate output value
					int32_t rel = uv_reli(adc, 0, scale);
					int32_t result = uv_lerpi(rel, 0, INT16_MAX);
					if (result < 0) {
						result = 0;
					}
					else if (result > INT16_MAX) {
						result = 0;
					}
					else {

					}
					// result should be negative on this side
					this->output16 = -result;
				}
				else {
					this->output16 = 0;
				}
			}
			// in middle output is 0
			else {
				this->output16 = 0;
			}
		}
	}
	else if (this->state == HALSENSOR_STATE_FAULT) {
		this->output16 = 0;
		// move back to ON state if read value is in middle
		if ((adc > this->config->middle - this->config->middle_tolerance / 2) &&
				(adc < this->config->middle + this->config->middle_tolerance / 2)) {
			this->state = HALSENSOR_STATE_ON;
		}
	}
	else if (this->state == HALSENSOR_STATE_CALIBRATION) {
		if (adc < this->config->min) {
			this->config->min = adc;
		}
		else if (adc > this->config->max) {
			this->config->max = adc;
		}
		else {

		}
		this->config->middle = adc;

		this->output16 = 0;
	}
	else {
		this->output16 = 0;
	}

	// lastly update output16 and output8
	this->output32 = this->output16 * 0x10000;
	this->output8 = this->output16 / 0x100;

	return this->output32;
}

void uv_halsensor_set_calbration(uv_halsensor_st *this, bool value) {
	if (this->state == HALSENSOR_STATE_CALIBRATION && !value) {
		this->state = HALSENSOR_STATE_ON;
	}
	else if (value) {
		if (this->state != HALSENSOR_STATE_CALIBRATION) {
			// reset the values to defaults
			int32_t adc = uv_adc_read(this->adc_chn);
			this->config->min = adc;
			this->config->max = adc;
			this->config->middle = adc;
		}
		this->state = HALSENSOR_STATE_CALIBRATION;
	}
	else {

	}
}




#endif