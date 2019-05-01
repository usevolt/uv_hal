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



#include "uv_ref_output.h"


#if CONFIG_REF_OUTPUT

#define PID_P_MAX			20000
#define TARGET_DELAY_MS		20
#define PID_MULTIPLIER		0x10
#define OUT_AVG_COUNT		20


void uv_ref_output_conf_reset(uv_ref_output_conf_st *conf, uv_ref_output_limitconf_st *limitconf) {
	conf->posmax_ppt = CONFIG_REF_POSMAX_PPT_DEF;
	conf->posmin_ppt = CONFIG_REF_POSMIN_PPT_DEF;
	conf->negmax_ppt = CONFIG_REF_NEGMAX_PPT_DEF;
	conf->negmin_ppt = CONFIG_REF_NEGMIN_PPT_DEF;
	conf->acc = CONFIG_REF_ACC_DEF;
	conf->dec = CONFIG_REF_DEC_DEF;
	conf->assembly_invert = 0;
	conf->invert = 0;
	limitconf->limit_max_mv = CONFIG_REF_LIMITMAX_MV_DEF;
	limitconf->limit_min_mv = CONFIG_REF_LIMITMIN_MV_DEF;
	limitconf->limit_max_ppt = CONFIG_REF_LIMITMAX_PPT_DEF;
	limitconf->limit_min_ppt = CONFIG_REF_LIMITMIN_PPT_DEF;
}

// small helper function to invert the PM signal
static void pwm_set(uv_ref_output_st *this, uint16_t dc) {
	if (dc > 1000) {
		dc = 1000;
	}
	uv_pwm_set(this->pwm_chn, 1000 - dc);
}

// helper function to measure the output value
static uint16_t get_mv(uv_ref_output_st *this) {
	int32_t mv = uv_adc_read(this->adc_chn) * this->adc_mult / 1000;
	return mv;
}




void uv_ref_output_init(uv_ref_output_st *this,
		uv_ref_output_conf_st *conf_ptr, uv_ref_output_limitconf_st *limitconf,
		uv_pwm_channel_t pwm_chn, uv_adc_channels_e adc_chn, uint16_t sense_ampl) {
	this->conf = conf_ptr;
	this->limitconf = limitconf;
	this->pwm_chn = pwm_chn;
	this->adc_chn = adc_chn;
	this->adc_mult = sense_ampl;
	this->state = REF_OUTPUT_STATE_ENABLED;
	this->mode = REF_OUTPUT_MODE_REL;
	this->out = 0;
	uv_moving_aver_init(&this->out_avg, OUT_AVG_COUNT);
	this->pwm = 0;
	uv_pid_init(&this->mv_pid, CONFIG_REF_PID_P_DEF, CONFIG_REF_PID_I_DEF, 0);
	uv_pid_set_target(&this->mv_pid, this->target);

	uv_pid_init(&this->target_pid, PID_P_MAX, 0, 0);
	uv_delay_init(&this->target_delay, TARGET_DELAY_MS);
	this->target = 0;
	this->target_mv = 0;
	this->target_req = 0;
	this->target_mult = 0;

	pwm_set(this, 0);
}




void uv_ref_output_step(uv_ref_output_st *this, uint16_t vdd_mv, uint16_t step_ms) {
	// check the conf limits
	if (this->limitconf->limit_max_ppt > 1000) {
		this->limitconf->limit_max_ppt = 1000;
	}
	if (this->conf->posmax_ppt > 1000) {
		this->conf->posmax_ppt = 1000;
	}
	if (this->conf->negmax_ppt > 1000) {
		this->conf->negmax_ppt = 1000;
	}
	if (this->conf->posmin_ppt > 1000) {
		this->conf->posmin_ppt = 1000;
	}
	if (this->conf->negmin_ppt > 1000) {
		this->conf->negmin_ppt = 1000;
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
		this->conf->assembly_invert = 0;
	}

	this->out = uv_moving_aver_step(&this->out_avg, get_mv(this)) *
			((this->conf->assembly_invert) ? -1 : 1) *
			((this->target < 0) ? -1 : 1);

	// target driving
	if (this->mode != REF_OUTPUT_MODE_ONOFFABS &&
			this->mode != REF_OUTPUT_MODE_ONOFFREL) {
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

			// in disabled state the PID drives to zero
			if (this->state == REF_OUTPUT_STATE_DISABLED) {
				this->target_req = 0;
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
	}
	else {
		// in onoff mode the PID controller is disabled and we put the output value directly
		if (this->target_req > 0) {
			this->target = 1000;
		}
		else if (this->target_req < 0) {
			this->target = -1000;
		}
		else {
			this->target = 0;
		}
	}


	if (this->state == REF_OUTPUT_STATE_ENABLED) {
		int32_t target_mv = 0;

		int16_t limit_min_mv;
		int16_t limit_max_mv;
		// mode is REF_OUTPUT_MODE_REL
		if (this->mode == REF_OUTPUT_MODE_REL ||
				this->mode == REF_OUTPUT_MODE_ONOFFREL) {
			limit_min_mv = vdd_mv * this->limitconf->limit_min_ppt / 1000;
			limit_max_mv = vdd_mv * this->limitconf->limit_max_ppt / 1000;
		}
		// mode is REF_OUTPUT_MODE_ABS
		else {
			limit_min_mv = this->limitconf->limit_min_mv;
			limit_max_mv = this->limitconf->limit_max_mv;
		}

		// target value is always the middle voltage between limit_max and limit_min, relative
		// to the supply voltage
		target_mv = (limit_max_mv - limit_min_mv) / 2 + limit_min_mv;

		if ((this->target * ((this->conf->assembly_invert) ? -1 : 1)) > 0) {
			// output value increases
			uint16_t min = this->conf->posmin_ppt;
			uint16_t max = this->conf->posmax_ppt;
			int32_t rel = uv_lerpi(abs(this->target), min, max);
			target_mv = uv_lerpi(rel, target_mv, limit_max_mv);
		}
		else if (this->target != 0) {
			// output value decreases
			uint16_t min = this->conf->negmin_ppt;
			uint16_t max = this->conf->negmax_ppt;
			int32_t rel = uv_lerpi(abs(this->target), min, max);
			target_mv = uv_lerpi(rel, target_mv, limit_min_mv);
		}
		else {

		}

		if (target_mv < 0) {
			target_mv = 0;
		}
		else if (target_mv > vdd_mv) {
			target_mv = vdd_mv;
		}
		else {

		}

		this->target_mv = target_mv;
		uv_pid_set_target(&this->mv_pid, target_mv);
		uv_pid_step(&this->mv_pid, step_ms, (int32_t) uv_moving_aver_get_val(&this->out_avg));
		int32_t pwmvalue = ((int32_t) this->pwm) + uv_pid_get_output(&this->mv_pid);

//		printf("%i %i\n", uv_pid_get_output(&this->mv_pid), this->out);

		if (pwmvalue < 0) {
			pwmvalue = 0;
		}
		else if (pwmvalue > PWM_MAX_VALUE) {
			pwmvalue = PWM_MAX_VALUE;
		}
		else {

		}
		this->pwm = pwmvalue;
		pwm_set(this, pwmvalue);
	}
	else {
		pwm_set(this, 0);
	}

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


void uv_ref_output_disable(uv_ref_output_st *this) {
	this->state = REF_OUTPUT_STATE_DISABLED;
	pwm_set(this, 0);

}


void uv_ref_output_enable(uv_ref_output_st *this) {
	this->state = REF_OUTPUT_STATE_ENABLED;
}




#endif

