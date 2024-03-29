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



#include "uv_ref_output.h"


#if CONFIG_REF_OUTPUT




// small helper function to assign the pwm value based on the look up table
static void pwm_set(uv_ref_output_st *this, int16_t rel_value) {
	int32_t pwm_val = 0;

	if (this->lookuptable_len >= 2) {
		const uv_ref_output_lookup_st *smaller = &this->lookuptable[this->lookuptable_len - 1],
				*bigger = &this->lookuptable[0];
		if (rel_value <= this->lookuptable[0].rel_value) {
			smaller = &this->lookuptable[0];
			bigger = smaller;
		}
		else if (rel_value >= this->lookuptable[this->lookuptable_len - 1].rel_value) {
			bigger = &this->lookuptable[this->lookuptable_len - 1];
			smaller = bigger;
		}
		else {
			// find closest smaller value
			for (int8_t i = this->lookuptable_len - 2; i >= 0; i--) {
				if (this->lookuptable[i].rel_value < rel_value) {
					smaller = &this->lookuptable[i];
					break;
				}
			}
			// find closest bigger value
			for (uint8_t i = 1; i < this->lookuptable_len; i++) {
				if (this->lookuptable[i].rel_value >= rel_value) {
					bigger = &this->lookuptable[i];
					break;
				}
			}
		}
		// interpolate value between found ones
		int32_t rel = 0;
		if (smaller->rel_value != bigger->rel_value) {
			rel = uv_reli(rel_value, smaller->rel_value, bigger->rel_value);
		}
		pwm_val = uv_lerpi(rel, smaller->pwm_value, bigger->pwm_value);
	}

	LIMITS(pwm_val, 0, PWM_MAX_VALUE);
	this->pwm = pwm_val;
	uv_pwm_set(this->pwm_chn, pwm_val);
}





void uv_ref_output_init(uv_ref_output_st *this,
		uv_prop_output_conf_st *conf, uv_prop_output_limitconf_st *limitconf,
		uv_pwm_channel_t pwm_chn, uint32_t emcy_fault,
		const uv_ref_output_lookup_st *lookup_table, uint8_t lookup_table_len) {

	uv_prop_output_init((uv_prop_output_st*) this, conf, limitconf);

	this->pwm_chn = pwm_chn;
	this->out = 0;
	this->pwm = 0;
	this->lookuptable = lookup_table;
	this->lookuptable_len = lookup_table_len;

	pwm_set(this, 0);
}




void uv_ref_output_step(uv_ref_output_st *this, uint16_t step_ms) {
	uv_prop_output_step((uv_prop_output_st *) this, step_ms);


	uv_prop_output_conf_st *conf = uv_prop_output_get_conf((uv_prop_output_st*) this);
	uv_prop_output_limitconf_st *limitconf =
			uv_prop_output_get_limitconf((uv_prop_output_st*) this);

	int16_t maxspeed_scaler = uv_ref_output_get_maxspeed_scaler(this);
	LIMITS(maxspeed_scaler, 0, 1000);

	// in disabled state target is always zero
	uv_output_state_e state = uv_prop_output_get_state((uv_prop_output_st*) this);
	uint16_t limit_max = limitconf->max,
			 limit_min = limitconf->min;
	 int32_t rel_value = (limit_max - limit_min) / 2 + limit_min;
	if (state != OUTPUT_STATE_ON) {
		// put the state to the middle value
		// middle value is always the middle value between limit_max and limit_min
		pwm_set(this, (limit_max - limit_min) / 2 + limit_min);
	}
	else {

		int16_t target = uv_prop_output_get_target((uv_prop_output_st *) this);

		if (target > 0) {
			int32_t rel = uv_lerpi(abs(target),
									conf->solenoid_conf[0].min,
									uv_lerpi(maxspeed_scaler,
											conf->solenoid_conf[0].min,
											conf->solenoid_conf[0].max));
			// rel is now 0 ... UINT8_MAX. Convert it to 0 ... 1000
			rel = uv_reli(rel, 0, SOLENOID_OUTPUT_CONF_MAX);
			rel_value = uv_lerpi(rel, rel_value, limit_max);
		}
		else if (target != 0) {
			// negative limit values
			int32_t rel = uv_lerpi(abs(target),
									conf->solenoid_conf[1].min,
									uv_lerpi(maxspeed_scaler,
											conf->solenoid_conf[1].min,
											conf->solenoid_conf[1].max));
			// rel is now 0 ... UINT8_MAX. Convert it to 0 ... 1000
			rel = uv_reli(rel, 0, SOLENOID_OUTPUT_CONF_MAX);
			rel_value = uv_lerpi(rel, rel_value, limit_min);
		}
		else {
		}

		LIMITS(rel_value, 0, 1000);
		pwm_set(this, rel_value);
	}

	this->out = uv_prop_output_get_target((uv_prop_output_st *) this);
}







#endif
