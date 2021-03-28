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
#include "sct_pwm_15xx.h"
#include "uv_gpio.h"

#if CONFIG_PWM


typedef struct {
	LPC_SCT_T *modules[4];
	uint32_t pwm_freq[4];
} pwm_st;
static pwm_st pwm;
#define this (&pwm)


uv_errors_e _uv_pwm_init() {
	this->modules[0] = LPC_SCT0;
	this->modules[1] = LPC_SCT1;
	this->modules[2] = LPC_SCT2;
	this->modules[3] = LPC_SCT3;

#if CONFIG_PWM0
	Chip_SCTPWM_Init(LPC_SCT0);
	Chip_SCTPWM_SetRate(LPC_SCT0, CONFIG_PWM0_FREQ);
	this->pwm_freq[0] = CONFIG_PWM0_FREQ;
#if CONFIG_PWM0_0
	Chip_SWM_MovablePortPinAssign(SWM_SCT0_OUT0_O,  UV_GPIO_PORT(CONFIG_PWM0_0_IO),
			UV_GPIO_PIN(CONFIG_PWM0_0_IO));
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 1, 0);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 1, 0);
#endif
#if CONFIG_PWM0_1
	Chip_SWM_MovablePortPinAssign(SWM_SCT0_OUT1_O,  UV_GPIO_PORT(CONFIG_PWM0_1_IO),
			UV_GPIO_PIN(CONFIG_PWM0_1_IO));
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 2, 1);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 2, 0);
#endif
#if CONFIG_PWM0_2
	Chip_SWM_MovablePortPinAssign(SWM_SCT0_OUT2_O,  UV_GPIO_PORT(CONFIG_PWM0_2_IO),
			UV_GPIO_PIN(CONFIG_PWM0_2_IO));
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 3, 2);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 3, 0);
#endif
#if CONFIG_PWM0_3
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT0_OUT3);
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 4, 3);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 4, 0);
#endif
#if CONFIG_PWM0_4
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT0_OUT4);
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 5, 4);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 5, 0);
#endif
#if CONFIG_PWM0_5
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT0_OUT5);
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 6, 5);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 6, 0);
#endif
#if CONFIG_PWM0_6
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT0_OUT6);
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 7, 6);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 7, 0);
#endif
#if CONFIG_PWM0_7
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT0_OUT7);
	Chip_SCTPWM_SetOutPin(LPC_SCT0, 8, 7);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT0, 8, 0);
#endif
	Chip_SCTPWM_Start(LPC_SCT0);
#endif
#if CONFIG_PWM1
	Chip_SCTPWM_Init(LPC_SCT1);
	Chip_SCTPWM_SetRate(LPC_SCT1, CONFIG_PWM1_FREQ);
	this->pwm_freq[1] = CONFIG_PWM1_FREQ;
#if CONFIG_PWM1_0
	Chip_SWM_MovablePortPinAssign(SWM_SCT1_OUT0_O,  UV_GPIO_PORT(CONFIG_PWM1_0_IO),
			UV_GPIO_PIN(CONFIG_PWM1_0_IO));
	Chip_SCTPWM_SetOutPin(LPC_SCT1, 1, 0);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT1, 1, 0);
#endif
#if CONFIG_PWM1_1
	Chip_SWM_MovablePortPinAssign(SWM_SCT1_OUT1_O,  UV_GPIO_PORT(CONFIG_PWM1_1_IO),
			UV_GPIO_PIN(CONFIG_PWM1_1_IO));
	Chip_SCTPWM_SetOutPin(LPC_SCT1, 2, 1);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT1, 2, 0);
#endif
#if CONFIG_PWM1_2
	Chip_SWM_MovablePortPinAssign(SWM_SCT1_OUT2_O,  UV_GPIO_PORT(CONFIG_PWM1_2_IO),
			UV_GPIO_PIN(CONFIG_PWM1_2_IO));
	Chip_SCTPWM_SetOutPin(LPC_SCT1, 3, 2);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT1, 3, 0);
#endif
#if CONFIG_PWM1_3
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT1_OUT3);
	Chip_SCTPWM_SetOutPin(LPC_SCT1, 4, 3);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT1, 4, 0);
#endif
#if CONFIG_PWM1_4
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT1_OUT4);
	Chip_SCTPWM_SetOutPin(LPC_SCT1, 5, 4);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT1, 5, 0);
#endif
#if CONFIG_PWM1_5
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT1_OUT5);
	Chip_SCTPWM_SetOutPin(LPC_SCT1, 6, 5);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT1, 6, 0);
#endif
#if CONFIG_PWM1_6
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT1_OUT6);
	Chip_SCTPWM_SetOutPin(LPC_SCT1, 7, 6);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT1, 7, 0);
#endif
#if CONFIG_PWM1_7
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT1_OUT7);
	Chip_SCTPWM_SetOutPin(LPC_SCT1, 8, 7);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT1, 8, 0);
#endif
	Chip_SCTPWM_Start(LPC_SCT1);
#endif
#if CONFIG_PWM2
	Chip_SCTPWM_Init(LPC_SCT2);
	Chip_SCTPWM_SetRate(LPC_SCT2, CONFIG_PWM0_FREQ);
	this->pwm_freq[2] = CONFIG_PWM2_FREQ;
#if CONFIG_PWM2_0
	Chip_SWM_MovablePortPinAssign(SWM_SCT2_OUT0_O,  UV_GPIO_PORT(CONFIG_PWM2_0_IO),
			UV_GPIO_PIN(CONFIG_PWM2_0_IO));
	Chip_SCTPWM_SetOutPin(LPC_SCT2, 1, 0);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT2, 1, 0);
#endif
#if CONFIG_PWM2_1
	Chip_SWM_MovablePortPinAssign(SWM_SCT2_OUT1_O,  UV_GPIO_PORT(CONFIG_PWM2_1_IO),
			UV_GPIO_PIN(CONFIG_PWM2_1_IO));
	Chip_SCTPWM_SetOutPin(LPC_SCT2, 2, 1);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT2, 2, 0);
#endif
#if CONFIG_PWM2_2
	Chip_SWM_MovablePortPinAssign(SWM_SCT2_OUT2_O,  UV_GPIO_PORT(CONFIG_PWM2_2_IO),
			UV_GPIO_PIN(CONFIG_PWM2_2_IO));
	Chip_SCTPWM_SetOutPin(LPC_SCT2, 3, 2);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT2, 3, 0);
#endif
#if CONFIG_PWM2_3
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT2_OUT3);
	Chip_SCTPWM_SetOutPin(LPC_SCT2, 4, 3);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT2, 4, 0);
#endif
#if CONFIG_PWM2_4
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT2_OUT4);
	Chip_SCTPWM_SetOutPin(LPC_SCT2, 5, 4);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT2, 5, 0);
#endif
#if CONFIG_PWM2_5
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT2_OUT5);
	Chip_SCTPWM_SetOutPin(LPC_SCT2, 6, 5);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT2, 6, 0);
#endif
	Chip_SCTPWM_Start(LPC_SCT2);
#endif
#if CONFIG_PWM3
	Chip_SCTPWM_Init(LPC_SCT3);
	Chip_SCTPWM_SetRate(LPC_SCT3, CONFIG_PWM3_FREQ);
	this->pwm_freq[3] = CONFIG_PWM3_FREQ;
#if CONFIG_PWM3_0
	Chip_SWM_MovablePortPinAssign(SWM_SCT3_OUT0_O,  UV_GPIO_PORT(CONFIG_PWM3_0_IO),
			UV_GPIO_PIN(CONFIG_PWM3_0_IO));
	Chip_SCTPWM_SetOutPin(LPC_SCT3, 1, 0);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT3, 1, 0);
#endif
#if CONFIG_PWM3_1
	Chip_SWM_MovablePortPinAssign(SWM_SCT3_OUT1_O,  UV_GPIO_PORT(CONFIG_PWM3_1_IO),
			UV_GPIO_PIN(CONFIG_PWM3_1_IO));
	Chip_SCTPWM_SetOutPin(LPC_SCT3, 2, 1);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT3, 2, 0);
#endif
#if CONFIG_PWM3_2
	Chip_SWM_MovablePortPinAssign(SWM_SCT3_OUT2_O,  UV_GPIO_PORT(CONFIG_PWM3_2_IO),
			UV_GPIO_PIN(CONFIG_PWM3_2_IO));
	Chip_SCTPWM_SetOutPin(LPC_SCT3, 3, 2);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT3, 3, 0);
#endif
#if CONFIG_PWM3_3
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT3_OUT3);
	Chip_SCTPWM_SetOutPin(LPC_SCT3, 4, 3);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT3, 4, 0);
#endif
#if CONFIG_PWM3_4
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT3_OUT4);
	Chip_SCTPWM_SetOutPin(LPC_SCT3, 5, 4);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT3, 5, 0);
#endif
#if CONFIG_PWM3_5
	Chip_SWM_EnableFixedPin(SWM_FIXED_SCT3_OUT5);
	Chip_SCTPWM_SetOutPin(LPC_SCT3, 6, 5);
	Chip_SCTPWM_SetDutyCycle(LPC_SCT3, 6, 0);
#endif
	Chip_SCTPWM_Start(LPC_SCT3);
#endif


	return ERR_NONE;
}



uv_errors_e uv_pwm_set(uv_pwm_channel_t chn, uint16_t value) {
	uv_errors_e ret = ERR_NONE;
	if (chn != 0) {
		if (value > PWM_MAX_VALUE) {
			value = PWM_MAX_VALUE;
		}
		Chip_SCTPWM_SetDutyCycle(this->modules[PWM_GET_MODULE(chn)], PWM_GET_CHANNEL(chn) + 1,
				Chip_SCTPWM_GetTicksPerCycle(this->modules[PWM_GET_MODULE(chn)]) * value / PWM_MAX_VALUE);
	}
	else {
		ret = ERR_UNSUPPORTED_PARAM1_VALUE;
	}

	return ret;
}


uint16_t uv_pwm_get(uv_pwm_channel_t chn) {
	uint16_t ret = 0;
	if (chn != 0) {
		ret = PWM_MAX_VALUE * Chip_SCTPWM_GetDutyCycle(this->modules[PWM_GET_MODULE(chn)],
				PWM_GET_CHANNEL(chn) + 1) /
				Chip_SCTPWM_GetTicksPerCycle(this->modules[PWM_GET_MODULE(chn)]);
	}

	return ret;
}



void uv_pwm_set_freq(uv_pwm_channel_t chn, uint32_t value) {
	if (chn != 0) {
		if (this->pwm_freq[PWM_GET_MODULE(chn)] != value) {
			Chip_SCTPWM_Stop(this->modules[PWM_GET_MODULE(chn)]);
			Chip_SCTPWM_SetRate(this->modules[PWM_GET_MODULE(chn)], value);
			Chip_SCTPWM_Start(this->modules[PWM_GET_MODULE(chn)]);
			this->pwm_freq[PWM_GET_MODULE(chn)] = value;
		}
	}
}


#endif
