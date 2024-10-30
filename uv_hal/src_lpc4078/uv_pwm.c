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


#include "uv_pwm.h"


#include "chip.h"
#include "timer_17xx_40xx.h"
#include "uv_gpio.h"
#include "uv_terminal.h"

#if CONFIG_PWM



// definition of external module interface
struct ext_module {
	void (*set_callb)(void *module_ptr, uint32_t chn, uint16_t value);
	uint16_t (*get_callb)(void *module_ptr, uint32_t chn);
	void (*setfreq_callb)(void *module_ptr, uint32_t chn, uint32_t freq);
	void *module_ptr;
};


typedef struct {
	LPC_TIMER_T *modules[4];
	uint32_t pwm_freq[4];
	struct {
		// stores match register value for each PWM channel.
		// This is updated when timer reaches 0
		uint32_t match[2];
	} pwm[2];
#if CONFIG_PWMEXT_MODULE_COUNT
	struct ext_module ext_module[CONFIG_PWMEXT_MODULE_COUNT];
#endif
} pwm_st;
static pwm_st pwm = {};
#define this (&pwm)


void TIMER0_IRQHandler(void) {
	// set PWM matches high
	if (this->pwm[0].match[0]) {
		LPC_TIMER0->EMR |= (1 << 0);
	}
	if (this->pwm[0].match[1]) {
		LPC_TIMER0->EMR |= (1 << 1);
	}
	Chip_TIMER_SetMatch(LPC_TIMER0, 0, this->pwm[0].match[0]);
	Chip_TIMER_SetMatch(LPC_TIMER0, 1, this->pwm[0].match[1]);
	Chip_TIMER_Enable(LPC_TIMER0);
}
void TIMER1_IRQHandler(void) {
	// set PWM matches high
	if (this->pwm[1].match[0]) {
		LPC_TIMER1->EMR |= (1 << 0);
	}
	if (this->pwm[1].match[1]) {
		LPC_TIMER1->EMR |= (1 << 1);
	}
	Chip_TIMER_SetMatch(LPC_TIMER1, 0, this->pwm[1].match[0]);
	Chip_TIMER_SetMatch(LPC_TIMER1, 1, this->pwm[1].match[1]);
	Chip_TIMER_Enable(LPC_TIMER1);
}
void TIMER2_IRQHandler(void) {
	// set PWM matches high
	if (this->pwm[2].match[0]) {
		LPC_TIMER2->EMR |= (1 << 0);
	}
	if (this->pwm[2].match[1]) {
		LPC_TIMER2->EMR |= (1 << 1);
	}
	Chip_TIMER_SetMatch(LPC_TIMER2, 0, this->pwm[2].match[0]);
	Chip_TIMER_SetMatch(LPC_TIMER2, 1, this->pwm[2].match[1]);
	Chip_TIMER_Enable(LPC_TIMER2);
}
void TIMER3_IRQHandler(void) {
	// set PWM matches high
	if (this->pwm[3].match[0]) {
		LPC_TIMER3->EMR |= (1 << 0);
	}
	if (this->pwm[3].match[1]) {
		LPC_TIMER3->EMR |= (1 << 1);
	}
	Chip_TIMER_SetMatch(LPC_TIMER3, 0, this->pwm[3].match[0]);
	Chip_TIMER_SetMatch(LPC_TIMER3, 1, this->pwm[3].match[1]);
	Chip_TIMER_Enable(LPC_TIMER3);
}



uv_errors_e _uv_pwm_init() {
	this->modules[0] = LPC_TIMER0;
	this->modules[1] = LPC_TIMER1;
	this->modules[2] = LPC_TIMER2;
	this->modules[3] = LPC_TIMER3;

	memset(&this->pwm, 0, sizeof(this->pwm));

#if CONFIG_PWM0
	Chip_TIMER_Init(LPC_TIMER0);
	this->pwm_freq[0] = CONFIG_PWM0_FREQ;
	// MATCH3 is used to generate interrupt to set PWM's high
	Chip_TIMER_SetMatch(LPC_TIMER0, 3, 0);
	Chip_TIMER_StopOnMatchEnable(LPC_TIMER0, 3);
	Chip_TIMER_MatchEnableInt(LPC_TIMER0, 3);
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
	NVIC_EnableIRQ(TIMER0_IRQn);

	// match 2 is used to reset timer value, also sets the frequency
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER0, 2);

	uv_pwm_set_freq(PWM0_0, CONFIG_PWM0_FREQ);
#if CONFIG_PWM0_0
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER0,
			0,
			TIMER_EXTMATCH_CLEAR, 0);
	Chip_TIMER_SetMatch(LPC_TIMER0, 0, 0);
#if CONFIG_PWM0_0_IO == P1_28
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 28, IOCON_FUNC3);
#elif CONFIG_PWM0_0_IO == P3_25
	Chip_IOCON_PinMuxSet(LPC_IOCON, 3, 25, IOCON_FUNC3);
#else
#error "CONFIG_PWM0_0_IO defines unsupported PWM output pin or is not defined"
#endif
#endif
#if CONFIG_PWM0_1
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER0,
			0,
			TIMER_EXTMATCH_CLEAR, 1);
	Chip_TIMER_SetMatch(LPC_TIMER0, 1, 0);
#if CONFIG_PWM0_1_IO == P1_29
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 29, IOCON_FUNC3);
#elif CONFIG_PWM0_1_IO == P3_26
	Chip_IOCON_PinMuxSet(LPC_IOCON, 3, 26, IOCON_FUNC3);
#else
#error "CONFIG_PWM0_1_IO defines unsupported PWM output pin or is not defined"
#endif
#endif
#if CONFIG_PWM0_2 || CONFIG_PWM0_3 || CONFIG_PWM0_4 || CONFIG_PWM0_5 || \
	CONFIG_PWM0_6 || CONFIG_PWM0_7
#error "LPC4078 only supports 2 outputs per PWM module"
#endif
	Chip_TIMER_Enable(LPC_TIMER0);
#endif
#if CONFIG_PWM1
	Chip_TIMER_Init(LPC_TIMER1);
	this->pwm_freq[1] = CONFIG_PWM1_FREQ;
	// MATCH3 is used to generate interrupt to set PWM's high
	Chip_TIMER_SetMatch(LPC_TIMER1, 3, 0);
	Chip_TIMER_StopOnMatchEnable(LPC_TIMER1, 3);
	Chip_TIMER_MatchEnableInt(LPC_TIMER1, 3);
	NVIC_ClearPendingIRQ(TIMER1_IRQn);
	NVIC_EnableIRQ(TIMER1_IRQn);
	// match 2 is used to reset timer value, also sets the frequency
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER1, 2);

	uv_pwm_set_freq(PWM1_0, CONFIG_PWM1_FREQ);
#if CONFIG_PWM1_0
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER1,
			0,
			TIMER_EXTMATCH_CLEAR, 0);
	Chip_TIMER_SetMatch(LPC_TIMER1, 0, 0);
#if CONFIG_PWM1_0_IO == P1_22
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 22, IOCON_FUNC3);
#elif CONFIG_PWM1_0_IO == P3_29
	Chip_IOCON_PinMuxSet(LPC_IOCON, 3, 29, IOCON_FUNC3);
#else
#error "CONFIG_PWM1_0_IO defines unsupported PWM output pin or is not defined"
#endif
#endif
#if CONFIG_PWM1_1
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER1,
			0,
			TIMER_EXTMATCH_CLEAR, 1);
	Chip_TIMER_SetMatch(LPC_TIMER1, 1, 0);
#if CONFIG_PWM1_1_IO == P1_25
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 25, IOCON_FUNC3);
#elif CONFIG_PWM1_1_IO == P3_30
	Chip_IOCON_PinMuxSet(LPC_IOCON, 3, 30, IOCON_FUNC3);
#else
#error "CONFIG_PWM1_1_IO defines unsupported PWM output pin or is not defined"
#endif
#endif
#if CONFIG_PWM1_2 || CONFIG_PWM1_3 || CONFIG_PWM1_4 || CONFIG_PWM1_5 || \
	CONFIG_PWM1_6 || CONFIG_PWM1_7
#error "LPC4078 only supports 2 outputs per PWM module"
#endif
	Chip_TIMER_Enable(LPC_TIMER1);
#endif
#if CONFIG_PWM2
	Chip_TIMER_Init(LPC_TIMER2);
	this->pwm_freq[2] = CONFIG_PWM2_FREQ;
	// MATCH3 is used to generate interrupt to set PWM's high
	Chip_TIMER_SetMatch(LPC_TIMER2, 3, 0);
	Chip_TIMER_StopOnMatchEnable(LPC_TIMER2, 3);
	Chip_TIMER_MatchEnableInt(LPC_TIMER2, 3);
	NVIC_ClearPendingIRQ(TIMER2_IRQn);
	NVIC_EnableIRQ(TIMER2_IRQn);
	// match 2 is used to reset timer value, also sets the frequency
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER2, 2);

	uv_pwm_set_freq(PWM2_0, CONFIG_PWM2_FREQ);
#if CONFIG_PWM2_0
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER2,
			0,
			TIMER_EXTMATCH_CLEAR, 0);
	Chip_TIMER_SetMatch(LPC_TIMER2, 0, 0);
#if CONFIG_PWM2_0_IO == P0_6
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 6, IOCON_FUNC3);
#elif CONFIG_PWM2_0_IO == P2_5
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 5, IOCON_FUNC3);
#elif CONFIG_PWM2_0_IO == P4_28
	Chip_IOCON_PinMuxSet(LPC_IOCON, 4, 28, IOCON_FUNC3);
#else
#error "CONFIG_PWM2_0_IO defines unsupported PWM output pin or is not defined"
#endif
#endif
#if CONFIG_PWM2_1
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER2,
			0,
			TIMER_EXTMATCH_CLEAR, 1);
	Chip_TIMER_SetMatch(LPC_TIMER2, 1, 0);
#if CONFIG_PWM2_1_IO == P2_4
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 4, IOCON_FUNC3);
#elif CONFIG_PWM2_1_IO == P4_29
	Chip_IOCON_PinMuxSet(LPC_IOCON, 4, 29, IOCON_FUNC3);
#else
#error "CONFIG_PWM2_1_IO defines unsupported PWM output pin or is not defined"
#endif
#endif
#if CONFIG_PWM2_3 || CONFIG_PWM2_4 || CONFIG_PWM2_5 || \
	CONFIG_PWM2_6 || CONFIG_PWM2_7
#error "LPC4078 only supports 3 outputs on TIMER2 PWM module"
#endif
	Chip_TIMER_Enable(LPC_TIMER2);
#endif
#if CONFIG_PWM3
	Chip_TIMER_Init(LPC_TIMER3);
	this->pwm_freq[3] = CONFIG_PWM3_FREQ;
	// MATCH3 is used to generate interrupt to set PWM's high
	Chip_TIMER_SetMatch(LPC_TIMER3, 3, 0);
	Chip_TIMER_StopOnMatchEnable(LPC_TIMER3, 3);
	Chip_TIMER_MatchEnableInt(LPC_TIMER3, 3);
	NVIC_ClearPendingIRQ(TIMER3_IRQn);
	NVIC_EnableIRQ(TIMER3_IRQn);
	// match 2 is used to reset timer value, also sets the frequency
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER3, 2);

	uv_pwm_set_freq(PWM3_0, CONFIG_PWM3_FREQ);
#if CONFIG_PWM3_0
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER3,
			0,
			TIMER_EXTMATCH_CLEAR, 0);
	Chip_TIMER_SetMatch(LPC_TIMER3, 0, 0);
#if CONFIG_PWM3_0_IO == P0_10
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 10, IOCON_FUNC3);
#elif CONFIG_PWM3_0_IO == P1_9
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 9, IOCON_FUNC3);
#elif CONFIG_PWM3_0_IO == P2_26
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 26, IOCON_FUNC3);
#else
#error "CONFIG_PWM3_0_IO defines unsupported PWM output pin or is not defined"
#endif
#endif
#if CONFIG_PWM3_1
	Chip_TIMER_ExtMatchControlSet(LPC_TIMER3,
			0,
			TIMER_EXTMATCH_CLEAR, 1);
	Chip_TIMER_SetMatch(LPC_TIMER3, 1, 0);
#if CONFIG_PWM3_1_IO == P0_11
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 11, IOCON_FUNC3);
#elif CONFIG_PWM3_1_IO == P1_8
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 8, IOCON_FUNC3);
#elif CONFIG_PWM3_1_IO == P2_27
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 27, IOCON_FUNC3);
#else
#error "CONFIG_PWM3_1_IO defines unsupported PWM output pin or is not defined"
#endif
#endif
#if CONFIG_PWM3_2 || CONFIG_PWM3_3 || CONFIG_PWM3_4 || CONFIG_PWM3_5 || \
	CONFIG_PWM3_6 || CONFIG_PWM3_7
#error "LPC4078 only supports 3 outputs on TIMER3 PWM module"
#endif
	Chip_TIMER_Enable(LPC_TIMER3);
#endif

	return ERR_NONE;
}



uv_errors_e uv_pwm_set(uv_pwm_channel_t chn, uint16_t value) {
	uv_errors_e ret = ERR_NONE;

	if (value > PWM_MAX_VALUE) {
		value = PWM_MAX_VALUE;
	}
	uint8_t module = PWMEXT_GET_MODULE(chn);
	if (module == PWMEXT_MODULE_THIS &&
			PWMEXT_GET_CHN(chn) != 0) {
		uint8_t m = PWM_GET_MODULE(chn);
		this->pwm[m].match[PWM_GET_CHANNEL(chn)] =
				this->modules[m]->MR[2] * value / PWM_MAX_VALUE;
	}
#if CONFIG_PWMEXT_MODULE_COUNT
	else if (module <= CONFIG_PWMEXT_MODULE_COUNT) {
		if (this->ext_module[module - 1].set_callb != NULL) {
			this->ext_module[module - 1].set_callb(
					this->ext_module[module - 1].module_ptr, PWMEXT_GET_CHN(chn), value);
		}
	}
#endif
	else {
		ret = ERR_UNSUPPORTED_PARAM1_VALUE;
	}

	return ret;
}


uint16_t uv_pwm_get(uv_pwm_channel_t c) {
	uint16_t ret = 0;
	uint8_t module = PWMEXT_GET_MODULE(c);
	uint8_t chn = PWM_GET_CHANNEL(c);
	if (module == PWMEXT_MODULE_THIS &&
			PWMEXT_GET_CHN(chn) != 0) {
		ret = PWM_MAX_VALUE * this->modules[module]->MR[chn] /
				this->modules[module]->MR[2];
	}
#if CONFIG_PWMEXT_MODULE_COUNT
	else if (module <= CONFIG_PWMEXT_MODULE_COUNT) {
		ret = this->ext_module[module - 1].get_callb(
				this->ext_module[module - 1].module_ptr, PWMEXT_GET_CHN(chn));
	}
#endif
	else {
	}

	return ret;
}



void uv_pwm_set_freq(uv_pwm_channel_t chn, uint32_t value) {
	uint8_t module = PWMEXT_GET_MODULE(chn);
	if (module == PWMEXT_MODULE_THIS &&
			PWMEXT_GET_CHN(chn) != 0) {
		if (this->pwm_freq[PWM_GET_MODULE(chn)] != value) {
			Chip_TIMER_SetMatch(this->modules[module], 2,
					Chip_Clock_GetSystemClockRate() * value);
			this->pwm_freq[PWM_GET_MODULE(chn)] = value;
			this->modules[module]->TC = 0;
		}
	}
#if CONFIG_PWMEXT_MODULE_COUNT
	else if (module <= CONFIG_PWMEXT_MODULE_COUNT) {
		this->ext_module[module - 1].setfreq_callb(
				this->ext_module[module - 1].module_ptr, PWMEXT_GET_CHN(chn), value);
	}
#endif
	else {

	}
}



#if CONFIG_PWMEXT_MODULE_COUNT
uv_errors_e uv_pwmext_module_init(
		uint8_t module_index,
		void *module_ptr,
		void (*set_callb)(void *module_ptr, uint32_t chn, uint16_t value),
		uint16_t (*get_callb)(void *module_ptr, uint32_t chn),
		void (*setfreq_callb)(void *module_ptr, uint32_t chn, uint32_t freq)) {
	uv_errors_e ret = ERR_NONE;

	if (module_index <= CONFIG_PWMEXT_MODULE_COUNT) {
		this->ext_module[module_index - 1].module_ptr = module_ptr;
		this->ext_module[module_index - 1].setfreq_callb = setfreq_callb;
		this->ext_module[module_index - 1].get_callb = get_callb;
		this->ext_module[module_index - 1].set_callb = set_callb;
	}
	else {
		printf("uv_pwm.c | uv_pwm_init_module: Tried to initialize PWM module with higer\n"
				"module index than supported. Define CONFIG_PWM_MODULE_COUNT with a value\n"
				"or at least of %i\n", module_index);
		ret = ERR_HARDWARE_NOT_SUPPORTED;
	}
	return ret;
}
#endif



#endif
