/*
 * uv_output.c
 *
 *  Created on: Oct 10, 2017
 *      Author: usevolt
 */



#include <uv_output.h>


#include <uv_canopen.h>



void uv_output_init(uv_output_st *this, const uv_adc_channels_e adc_chn, const uv_gpios_e gate_io,
		const uint16_t sense_mohm, const uint16_t min_val, const uint16_t max_val,
		const uint16_t moving_avg_count, uint32_t emcy_overload, uint32_t emcy_fault) {
	this->adc_chn = adc_chn;
	this->gate_io = gate_io;
	uv_gpio_init_output(this->gate_io, false);
	this->sense_mohm = sense_mohm;
	this->sense_ampl = 50;
	this->limit_min = min_val;
	this->limit_max = max_val;
	uv_moving_aver_init(&this->moving_avg, moving_avg_count);
	this->emcy_overload = emcy_overload;
	this->emcy_fault = emcy_fault;
	this->state = OUTPUT_STATE_OFF;
	this->current = 0;
}


/// @brief: Sets the output state
void uv_output_set_state(uv_output_st *this, const uv_output_state_e state) {
	if ((this->state != OUTPUT_STATE_FAULT) &&
			(this->state != OUTPUT_STATE_OVERLOAD)) {
#if CONFIG_CANOPEN
		if ((state == OUTPUT_STATE_FAULT) && (this->emcy_fault)) {
			uv_canopen_emcy_send(CANOPEN_EMCY_DEVICE_SPECIFIC, this->emcy_fault);
		}
		else if ((state == OUTPUT_STATE_OVERLOAD) && (this->emcy_overload)) {
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


/// @brief: Step function should be called every step cycle.
void uv_output_step(uv_output_st *this, uint16_t step_ms) {
	int16_t adc = uv_adc_read(this->adc_chn) - 0x24;
	if (adc < 0) {
		adc = 0;
	}
	int32_t mv = (int32_t) adc * 3300 / ADC_MAX_VALUE ;
	// current is multiplied by inverted resistor value (1 / 0.002) and divided
	// by current sensing amplification
	int32_t current = mv * (1000 / (this->sense_mohm * this->sense_ampl));
	if (current < 0) {
		current = 0;
	}
	this->current = uv_moving_aver_step(&this->moving_avg, current);

	if (this->state == OUTPUT_STATE_ON) {
		if ((this->current > this->limit_max)) {
			uv_gpio_set(this->gate_io, false);
			uv_output_set_state(this, OUTPUT_STATE_OVERLOAD);
		}
		else if (this->current < this->limit_min) {
			uv_gpio_set(this->gate_io, false);
			uv_output_set_state(this, OUTPUT_STATE_FAULT);
		}
		else {
			uv_gpio_set(this->gate_io, true);
		}
	}
	else if (this->state == OUTPUT_STATE_OVERLOAD) {
		uv_gpio_set(this->gate_io, false);
	}
	else if (this->state == OUTPUT_STATE_FAULT) {
		uv_gpio_set(this->gate_io, false);
	}
	else {
		uv_gpio_set(this->gate_io, false);
	}

}






