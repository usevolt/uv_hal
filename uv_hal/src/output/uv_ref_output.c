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

#define PID_P_MAX			20000
#define TARGET_DELAY_MS		20
#define PID_MULTIPLIER		0x10
#define OUT_AVG_COUNT		20
#define TOGGLE_HYSTERESIS	100


void uv_ref_output_conf_reset(uv_ref_output_conf_st *conf, uv_ref_output_limitconf_st *limitconf) {
	conf->solenoid_conf[0].max_ppt = CONFIG_REF_POSMAX_PPT_DEF;
	conf->solenoid_conf[0].min_ppt = CONFIG_REF_POSMIN_PPT_DEF;
	conf->solenoid_conf[1].max_ppt = CONFIG_REF_NEGMAX_PPT_DEF;
	conf->solenoid_conf[1].min_ppt = CONFIG_REF_NEGMIN_PPT_DEF;
	conf->acc = CONFIG_REF_ACC_DEF;
	conf->dec = CONFIG_REF_DEC_DEF;
	conf->assembly_invert = 0;
	conf->invert = 0;
	limitconf->solenoid_limitconf[0].max_ppt = CONFIG_REF_LIMITMAX_PPT_DEF;
	limitconf->solenoid_limitconf[0].max_ma = 0;
	limitconf->solenoid_limitconf[1].max_ppt = CONFIG_REF_LIMITMIN_PPT_DEF;
	limitconf->solenoid_limitconf[1].max_ma = 0;
}

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
		uv_ref_output_conf_st *conf_ptr, uv_ref_output_limitconf_st *limitconf,
		uv_pwm_channel_t pwm_chn, uv_adc_channels_e adc_chn, uint16_t sense_ampl,
		uint16_t fault_ma, uint32_t emcy_fault,
		const uv_ref_output_lookup_st *lookup_table, uint8_t lookup_table_len) {

	uv_output_init((uv_output_st*) this, adc_chn, 0,
			sense_ampl, fault_ma, fault_ma, 5, 0, emcy_fault);
	uv_output_set_state((uv_output_st*) this, OUTPUT_STATE_ON);

	this->conf = conf_ptr;
	this->limitconf = limitconf;
	this->pwm_chn = pwm_chn;
	this->mode = DUAL_SOLENOID_OUTPUT_MODE_PROP_NORMAL;
	this->out = 0;
	this->pwm = 0;
	uv_pid_init(&this->mv_pid, CONFIG_REF_PID_P_DEF, CONFIG_REF_PID_I_DEF, 0);
	uv_pid_set_target(&this->mv_pid, this->target);
	this->lookuptable = lookup_table;
	this->lookuptable_len = lookup_table_len;
	uv_pid_init(&this->target_pid, PID_P_MAX, 0, 0);
	uv_delay_init(&this->target_delay, TARGET_DELAY_MS);
	this->target = 0;
	this->target_req = 0;
	this->target_mult = 0;
	uv_hysteresis_init(&this->toggle_hyst,
			REF_OUTPUT_TOGGLE_THRESHOLD_DEFAULT, TOGGLE_HYSTERESIS, false);
	this->last_hyst = 0;
	this->toggle_limit_ms = REF_OUTPUT_LIMIT_MS_DEFAULT;
	uv_delay_init(&this->toggle_delay, this->toggle_limit_ms);
	this->toggle_on = 0;

	pwm_set(this, 0);
}




void uv_ref_output_step(uv_ref_output_st *this, uint16_t step_ms) {
	// check the conf limits
	LIMITS(this->limitconf->solenoid_limitconf[0].max_ppt, 100, 1000);
	LIMITS(this->limitconf->solenoid_limitconf[1].max_ppt, 100, 1000);
	if (this->conf->solenoid_conf[0].max_ppt > 1000) {
		this->conf->solenoid_conf[0].max_ppt = 1000;
	}
	if (this->conf->solenoid_conf[1].max_ppt > 1000) {
		this->conf->solenoid_conf[1].max_ppt = 1000;
	}
	if (this->conf->solenoid_conf[0].min_ppt > 1000) {
		this->conf->solenoid_conf[0].min_ppt = 1000;
	}
	if (this->conf->solenoid_conf[1].min_ppt > 1000) {
		this->conf->solenoid_conf[1].min_ppt = 1000;
	}
	if (this->conf->acc > REF_OUTPUT_ACC_MAX) {
		this->conf->acc = REF_OUTPUT_ACC_MAX;
	}
	else if (this->conf->acc < REF_OUTPUT_ACC_MIN) {
		this->conf->acc = REF_OUTPUT_ACC_MIN;
	}
	else {

	}
	if (this->conf->dec > REF_OUTPUT_DEC_MAX) {
		this->conf->dec = REF_OUTPUT_DEC_MAX;
	}
	else if (this->conf->dec < REF_OUTPUT_DEC_MIN) {
		this->conf->dec = REF_OUTPUT_DEC_MIN;
	}
	else {

	}
	if (this->conf->assembly_invert > 1) {
		this->conf->assembly_invert = 1;
	}

	int32_t rel_value = 500;

	// in disabled state target is always zero
	if (uv_output_get_state((uv_output_st*) this) != OUTPUT_STATE_ON) {
		this->target = 0;
		uv_output_state_e state = uv_output_get_state((uv_output_st*) this);
		if (state == OUTPUT_STATE_DISABLED ||
				state == OUTPUT_STATE_OFF) {
			// put the state to the middle value
			pwm_set(this, 500);
		}
		else {
			// fault state puts the output to zero.
			// This should never happen if the system works as it should
			uv_pwm_set(this->pwm_chn, 0);
		}
		if (this->target_req == 0) {
			// put the output off when the request has ended. This
			// clears the fault states in case of a short circuit.
			uv_output_set_state((uv_output_st *) this, OUTPUT_STATE_OFF);
		}
		else if (uv_output_get_state((uv_output_st*) this) == OUTPUT_STATE_OFF) {
			// otherwise put the state back to ON
			uv_output_set_state((uv_output_st*) this, OUTPUT_STATE_ON);
		}
	}
	else {
		if (this->mode == DUAL_SOLENOID_OUTPUT_MODE_PROP_NORMAL) {
			if (uv_delay(&this->target_delay, step_ms)) {
				uv_delay_init(&this->target_delay, TARGET_DELAY_MS);

				if (this->conf->acc > 100) {
					this->conf->acc = 100;
				}
				if (this->conf->dec > 100) {
					this->conf->dec = 100;
				}
				uint16_t acc = this->conf->acc;
				uint16_t dec = this->conf->dec;

				if (this->target_req > REF_OUTPUT_VALUE_MAX) {
					this->target_req = REF_OUTPUT_VALUE_MAX;
				}
				else if (this->target_req < REF_OUTPUT_VALUE_MIN) {
					this->target_req = REF_OUTPUT_VALUE_MIN;
				}
				else {

				}

				// when acc and dec are maximum, bypass the pid controller to have
				// minimum delays
				if (acc == REF_OUTPUT_ACC_MAX &&
						dec == REF_OUTPUT_DEC_MAX) {
					this->target = this->target_req;
				}
				else {
					// different moving average values for accelerating and decelerating
					// maximum decelerating time is 1 sec
					if ((abs(this->target_req) > abs(this->target)) ||
							((int32_t) this->target_req * this->target < 0)) {
						// accelerating
						uv_pid_set_p(&this->target_pid, (uint32_t) PID_P_MAX * acc * acc / 10000);
					}
					else {
						// decelerating
						uv_pid_set_p(&this->target_pid, (uint32_t) PID_P_MAX * dec * dec / 10000);
					}

					uv_pid_set_target(&this->target_pid, this->target_req * PID_MULTIPLIER);
					uv_pid_step(&this->target_pid, TARGET_DELAY_MS, this->target_mult);
					this->target_mult += uv_pid_get_output(&this->target_pid);
					if ((this->target_mult / PID_MULTIPLIER) > 1000) {
						this->target_mult = 1000;
					}
					else if ((this->target_mult / PID_MULTIPLIER) < -1000) {
						this->target_mult = -1000;
					}
					else {

					}
					this->target = this->target_mult / PID_MULTIPLIER;

					// clamp output to target value when we're close enough
					if (uv_pid_get_output(&this->target_pid) == 0) {
						this->target = this->target_req;
						this->target_mult = this->target_req * PID_MULTIPLIER;
					}
				}
			}
			// since limits are given in uv_dual_solenoid_output format, limitmin is 0 ... 1000
			// which should represent the output value between 500 ... 0.
			uint16_t limit_max = this->limitconf->solenoid_limitconf[0].max_ppt,
					limit_min = 500 - this->limitconf->solenoid_limitconf[1].max_ppt / 2;
			// target value is always the middle value between limit_max and limit_min
			rel_value = (limit_max - limit_min) / 2 + limit_min;

			if ((this->target * ((this->conf->assembly_invert) ? -1 : 1)) > 0) {
				// output value increases
				int32_t rel = uv_lerpi(abs(this->target),
						this->conf->solenoid_conf[0].min_ppt,
						this->conf->solenoid_conf[0].max_ppt);
				rel_value = uv_lerpi(rel, rel_value, limit_max);
			}
			else if (this->target != 0) {
				// output value decreases
				int32_t rel = uv_lerpi(abs(this->target),
						this->conf->solenoid_conf[1].min_ppt,
						this->conf->solenoid_conf[1].max_ppt);
				rel_value = uv_lerpi(rel, rel_value, limit_min);
			}
			else {
			}
		}
		else {
			// ONOFF_NORMAL MODE
			uv_hysteresis_step(&this->toggle_hyst, abs(this->target_req));
			if (this->mode == DUAL_SOLENOID_OUTPUT_MODE_ONOFF_NORMAL) {
				// in onoff mode the PID controller is disabled and we put the output value directly
				if (uv_hysteresis_get_output(&this->toggle_hyst) && this->target_req) {
					this->target = (this->target_req > 0) ? 1000 : -1000;
				}
				else {
					this->target = 0;
					this->toggle_hyst.result = 0;
				}
			}
			// TOGGLE modes
			else {
				if (uv_hysteresis_get_output(&this->toggle_hyst) && this->target_req) {
					if (!this->last_hyst) {
						if (this->target_req > 0) {
							if (this->toggle_on == -1) {
								this->toggle_on = 1;
							}
							else {
								this->toggle_on = (this->toggle_on) ? 0 : 1;
							}
						}
						else {
							if (this->toggle_on == 1) {
								this->toggle_on = -1;
							}
							else {
								this->toggle_on = (this->toggle_on) ? 0 : -1;
							}
						}
					}
					uv_delay_init(&this->toggle_delay, this->toggle_limit_ms);
				}
				else {
					this->toggle_hyst.result = 0;
				}
				if (this->toggle_limit_ms && uv_delay(&this->toggle_delay, step_ms)) {
					this->toggle_on = 0;
				}
				this->target = (this->toggle_on) ? ((this->toggle_on > 0) ? 1000 : -1000) : 0;
				this->last_hyst = uv_hysteresis_get_output(&this->toggle_hyst);
			}
			// in both onoff modes the output is always either maximum or minimum.
			// This is to comply with solenoid_output modules
			if (this->target > 0) {
				rel_value = (this->mode == DUAL_SOLENOID_OUTPUT_MODE_PROP_TOGGLE) ?
						this->limitconf->solenoid_limitconf[0].max_ppt : 1000;
			}
			else if (this->target < 0) {
				rel_value = (this->mode == DUAL_SOLENOID_OUTPUT_MODE_PROP_TOGGLE) ?
						this->limitconf->solenoid_limitconf[1].max_ppt : 0;
			}
			else {
				rel_value = 500;
			}
		}


		LIMITS(rel_value, 0, 1000);
		pwm_set(this, rel_value);
	}

	this->out = this->target *
			(((int16_t) this->conf->assembly_invert) ? -1 : 1);

}


void uv_ref_output_set(uv_ref_output_st *this, int16_t value) {
	if (value > 1000) {
		value = 1000;
	}
	else if (value < -1000) {
		value = -1000;
	}
	else {

	}
	this->target_req = value;
}






#endif

