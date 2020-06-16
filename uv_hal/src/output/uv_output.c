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



#include <uv_output.h>


#include <uv_canopen.h>

#if CONFIG_OUTPUT


// default current calculation function
static uint16_t current_func(void *this_ptr, uint16_t adc) {
	int32_t current = (int32_t) adc * ((uv_output_st*) this_ptr)->sense_ampl / 1000;

	return current;
}


/// @brief: Sets the output GPIO if one is assigned. When extending
/// different output modules from this, set gate_io to 0.
static inline void set_out(uv_output_st *this, uint16_t value) {
	if (this->gate_io) {
		uv_gpio_set(this->gate_io, value);
	}
}


void uv_output_init(uv_output_st *this,  uv_adc_channels_e adc_chn, uv_gpios_e gate_io,
		uint16_t sense_ampl, uint16_t max_val_ma, uint16_t fault_val_ma,
		uint16_t moving_avg_count, uint32_t emcy_overload, uint32_t emcy_fault) {
	this->current_func = &current_func;
	this->adc_chn = adc_chn;
	this->gate_io = gate_io;
	if (this->gate_io) {
		uv_gpio_init_output(this->gate_io, false);
	}
	set_out(this, 0);
	this->sense_ampl = sense_ampl;
	this->limit_max_ma = max_val_ma;
	this->limit_fault_ma = fault_val_ma;
	uv_moving_aver_init(&this->moving_avg, moving_avg_count);
	uv_delay_end(&this->fault_freeze_delay);
	this->emcy_overload = emcy_overload;
	this->emcy_fault = emcy_fault;
	this->state = OUTPUT_STATE_OFF;
	this->current = 0;
}


/// @brief: Sets the output state
void uv_output_set_state(uv_output_st *this, const uv_output_state_e state) {
	// stat can be changed only if output is not disabled
	if (this->state != OUTPUT_STATE_DISABLED) {
		if ((this->state != OUTPUT_STATE_FAULT) &&
				(this->state != OUTPUT_STATE_OVERLOAD)) {
#if CONFIG_CANOPEN
			if ((state == OUTPUT_STATE_FAULT) &&
					(this->emcy_fault)) {
				uv_canopen_emcy_send(CANOPEN_EMCY_DEVICE_SPECIFIC, this->emcy_fault);
			}
			else if ((state == OUTPUT_STATE_OVERLOAD) &&
					(this->emcy_overload)) {
				uv_canopen_emcy_send(CANOPEN_EMCY_DEVICE_SPECIFIC, this->emcy_overload);
			}
			else {

			}
#endif
			this->state = state;
		}
		else if (state == OUTPUT_STATE_OFF) {
			this->state = state;
		}
		else {

		}
	}
}


void uv_output_enable(uv_output_st *this) {
	if (this->state == OUTPUT_STATE_DISABLED) {
		this->state = OUTPUT_STATE_OFF;
	}
}


/// @brief: Step function should be called every step cycle.
void uv_output_step(uv_output_st *this, uint16_t step_ms) {

	if (this->state == OUTPUT_STATE_ON) {

		int32_t current = 0;
		// current sense feedback
		if (this->adc_chn) {
			int32_t adc = uv_adc_read(this->adc_chn);
			if (adc < 0) {
				adc = 0;
			}
			current = this->current_func(this, adc);
			uv_moving_aver_step(&this->moving_avg, current);
		}

		this->current = uv_moving_aver_get_val(&this->moving_avg);

		// fault detection should be performed from the not-averaged current value
		uv_delay(&this->fault_freeze_delay, step_ms);
		if (uv_delay_has_ended(&this->fault_freeze_delay) &&
				(current > this->limit_fault_ma)) {
			set_out(this, false);
			uv_output_set_state(this, OUTPUT_STATE_FAULT);
		}
		// overcurrent detection is disabled for a short time during power on
		else if (uv_moving_aver_is_full(&this->moving_avg)) {
			if ((this->current > this->limit_max_ma)) {
				set_out(this, false);
				uv_output_set_state(this, OUTPUT_STATE_OVERLOAD);
			}
			else {
				set_out(this, true);
			}
		}
		else {
			set_out(this, true);
		}
	}
	else {
		this->current = 0;
		uv_moving_aver_reset(&this->moving_avg);
		set_out(this, false);
	}
}





#endif
